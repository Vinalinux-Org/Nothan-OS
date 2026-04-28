/* ============================================================
 * fat32.c
 * ------------------------------------------------------------
 * FAT32 filesystem driver — 8.3 names, root dir only.
 * ============================================================ */

/* INVARIANT: all sector I/O goes through the buffer cache.
 * sector_buf is scratch for BPB + initial root-dir scan only,
 * before any concurrent reader can exist. */

#include "types.h"
#include "fat32.h"
#include "vfs.h"
#include "vinix/blkdev.h"
#include "buffer_cache.h"
#include "uart.h"
#include "string.h"
#include "syscalls.h"

#define FAT32_SECTOR_SZ             512
#define FAT32_DIR_ENTRY_SZ          32
#define FAT32_MAX_FILES             32

/* Chain terminator — any value ≥ this marks end-of-chain */
#define FAT32_EOC                   0x0FFFFFF8
#define FAT32_BAD_CLUSTER           0x0FFFFFF7

/* BPB (BIOS Parameter Block) field offsets — from Microsoft FAT spec */
#define BPB_BytsPerSec              11
#define BPB_SecPerClus              13
#define BPB_RsvdSecCnt              14
#define BPB_NumFATs                 16
#define BPB_FATSz32                 36
#define BPB_RootClus                44
#define BPB_FSInfo                  48
#define BPB_Signature               510

/* 32-byte directory entry field offsets */
#define DIR_Name                    0
#define DIR_Attr                    11
#define DIR_NTRes                   12
#define DIR_FstClusHI               20
#define DIR_FstClusLO               26
#define DIR_FileSize                28

/* DIR_NTRes is a Microsoft extension: preserves case of 8.3 names
 * that would otherwise always appear uppercase on disk. */
#define NT_RES_LOWER_BASE           0x08
#define NT_RES_LOWER_EXT            0x10

#define ATTR_READ_ONLY              0x01
#define ATTR_HIDDEN                 0x02
#define ATTR_SYSTEM                 0x04
#define ATTR_VOLUME_ID              0x08
#define ATTR_DIRECTORY              0x10
#define ATTR_ARCHIVE                0x20
#define ATTR_LONG_NAME              0x0F

#define DIR_ENTRY_END               0x00
#define DIR_ENTRY_DELETED           0xE5

static struct {
    uint32_t part_lba;
    uint32_t fat_lba;
    uint32_t data_lba;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    uint32_t bytes_per_sector;
    uint32_t root_cluster;
    uint32_t fat_sectors;
    uint32_t num_fats;
    uint32_t fsinfo_lba;
} fs;

struct fat32_file {
    bool     in_use;
    char     name[13];          /* "hello.txt\0" for display */
    uint8_t  raw_name[11];      /* "HELLO   TXT" on-disk form */
    uint8_t  nt_res;
    uint32_t first_cluster;
    uint32_t size;
    uint32_t dir_entry_lba;     /* Where the on-disk entry lives, for write-back */
    uint32_t dir_entry_offset;
};

static struct fat32_file file_table[FAT32_MAX_FILES];
static int               file_count;

static struct gendisk *fs_bdev = 0;

/* Scratch buffer for bootstrap reads (BPB parse + initial scan). */
static uint8_t sector_buf[FAT32_SECTOR_SZ];

static int fat_read_sector(uint32_t lba, void *dst)
{
    struct buffer_head *bh = bread(fs_bdev, lba);
    if (!bh) return E_FAIL;
    memcpy(dst, bh->data, FAT32_SECTOR_SZ);
    brelse(bh);
    return E_OK;
}

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint32_t fat32_min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

/* Valid data clusters start at 2; 0 and 1 are reserved in FAT. */
static uint32_t cluster_to_sector(uint32_t cluster)
{
    return fs.data_lba + (cluster - 2) * fs.sectors_per_cluster;
}

/* Returns the raw FAT entry for a cluster (masked to 28 bits).
 * Sector caching is handled transparently by the buffer cache. */
static uint32_t fat_next_cluster(uint32_t cur)
{
    uint32_t fat_byte_off   = cur * 4;
    uint32_t fat_sector_idx = fat_byte_off / FAT32_SECTOR_SZ;
    uint32_t sector_off     = fat_byte_off % FAT32_SECTOR_SZ;
    uint32_t abs_sector     = fs.fat_lba + fat_sector_idx;

    struct buffer_head *bh = bread(fs_bdev, abs_sector);
    if (!bh) {
        pr_err("[FAT32] ERROR: FAT read failed at sector %u\n", abs_sector);
        return FAT32_EOC;
    }
    uint32_t entry = rd32(&bh->data[sector_off]);
    brelse(bh);

    /* Top 4 bits are reserved per spec — must be ignored on read. */
    return entry & 0x0FFFFFFF;
}

/* 8.3 name conversion: on-disk form is always 11 uppercase bytes
 * space-padded ("HELLO   TXT"). Display form uses NTRes flags for case. */

static char to_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

static char to_lower(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

/* "hello.txt" → "HELLO   TXT" (space-padded, uppercase). */
static void name_to_raw(const char *in, uint8_t raw[11])
{
    int i, pos;

    for (i = 0; i < 11; i++) {
        raw[i] = ' ';
    }

    pos = 0;
    while (in[pos] != '\0' && in[pos] != '.' && pos < 8) {
        raw[pos] = (uint8_t)to_upper(in[pos]);
        pos++;
    }

    while (in[pos] != '\0' && in[pos] != '.') {
        pos++;
    }

    if (in[pos] == '.') {
        pos++;
        for (i = 0; i < 3 && in[pos] != '\0'; i++, pos++) {
            raw[8 + i] = (uint8_t)to_upper(in[pos]);
        }
    }
}

/* "HELLO   TXT" → "hello.txt" / "HELLO.TXT" depending on NTRes flags. */
static void raw_to_name(const uint8_t raw[11], uint8_t nt_res, char out[13])
{
    int i, pos;
    bool lower_base = (nt_res & NT_RES_LOWER_BASE) != 0;
    bool lower_ext  = (nt_res & NT_RES_LOWER_EXT) != 0;

    pos = 0;

    for (i = 0; i < 8; i++) {
        if (raw[i] == ' ') break;
        out[pos++] = lower_base ? to_lower((char)raw[i]) : (char)raw[i];
    }

    if (raw[8] != ' ') {
        out[pos++] = '.';
        for (i = 0; i < 3; i++) {
            if (raw[8 + i] == ' ') break;
            out[pos++] = lower_ext ? to_lower((char)raw[8 + i]) : (char)raw[8 + i];
        }
    }

    out[pos] = '\0';
}

static bool raw_name_eq(const uint8_t a[11], const uint8_t b[11])
{
    int i;
    for (i = 0; i < 11; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static int parse_bpb(void)
{
    if (fat_read_sector(fs.part_lba, sector_buf) != E_OK) {
        pr_err("[FAT32] ERROR: Failed to read BPB\n");
        return E_FAIL;
    }

    /* BPB signature 0xAA55 is at offset 510, same value as MBR signature
     * but a separate occurrence at the partition boot sector. */
    if (rd16(&sector_buf[BPB_Signature]) != 0xAA55) {
        pr_err("[FAT32] ERROR: Invalid BPB signature\n");
        return E_FAIL;
    }

    fs.bytes_per_sector     = rd16(&sector_buf[BPB_BytsPerSec]);
    fs.sectors_per_cluster  = sector_buf[BPB_SecPerClus];
    fs.num_fats             = sector_buf[BPB_NumFATs];
    fs.fat_sectors          = rd32(&sector_buf[BPB_FATSz32]);
    fs.root_cluster         = rd32(&sector_buf[BPB_RootClus]);

    uint32_t reserved = rd16(&sector_buf[BPB_RsvdSecCnt]);
    uint32_t fsinfo   = rd16(&sector_buf[BPB_FSInfo]);

    if (fs.bytes_per_sector != FAT32_SECTOR_SZ) {
        pr_err("[FAT32] ERROR: Unsupported sector size %u\n", fs.bytes_per_sector);
        return E_FAIL;
    }

    fs.bytes_per_cluster = fs.sectors_per_cluster * fs.bytes_per_sector;
    fs.fat_lba           = fs.part_lba + reserved;
    fs.data_lba          = fs.fat_lba + fs.num_fats * fs.fat_sectors;
    fs.fsinfo_lba        = fs.part_lba + fsinfo;

    pr_info("[FAT32] BPB: bps=%u spc=%u nfats=%u fat_sz=%u root=%u\n",
                fs.bytes_per_sector, fs.sectors_per_cluster,
                fs.num_fats, fs.fat_sectors, fs.root_cluster);
    pr_info("[FAT32] Layout: fat_lba=%u data_lba=%u fsinfo_lba=%u\n",
                fs.fat_lba, fs.data_lba, fs.fsinfo_lba);

    return E_OK;
}

static int scan_root_dir(void)
{
    uint32_t cluster = fs.root_cluster;
    int done = 0;

    file_count = 0;

    while (!done && cluster < FAT32_EOC && cluster != 0) {
        uint32_t base_sector = cluster_to_sector(cluster);
        uint32_t s;

        for (s = 0; s < fs.sectors_per_cluster && !done; s++) {
            uint32_t abs_sector = base_sector + s;

            if (fat_read_sector(abs_sector, sector_buf) != E_OK) {
                pr_err("[FAT32] ERROR: Failed to read dir sector %u\n", abs_sector);
                return E_FAIL;
            }

            uint32_t off;
            for (off = 0; off < FAT32_SECTOR_SZ; off += FAT32_DIR_ENTRY_SZ) {
                uint8_t *entry = &sector_buf[off];
                uint8_t first = entry[DIR_Name];
                uint8_t attr  = entry[DIR_Attr];

                if (first == DIR_ENTRY_END) {
                    done = 1;
                    break;
                }
                if (first == DIR_ENTRY_DELETED) {
                    continue;
                }
                if (attr == ATTR_LONG_NAME) {
                    continue;
                }
                if (attr & (ATTR_VOLUME_ID | ATTR_DIRECTORY)) {
                    continue;
                }

                if (file_count >= FAT32_MAX_FILES) {
                    pr_info("[FAT32] WARNING: file_table full, truncating\n");
                    done = 1;
                    break;
                }

                struct fat32_file *f = &file_table[file_count];
                int k;

                f->in_use = true;
                for (k = 0; k < 11; k++) {
                    f->raw_name[k] = entry[DIR_Name + k];
                }
                f->nt_res = entry[DIR_NTRes];
                raw_to_name(f->raw_name, f->nt_res, f->name);

                uint16_t hi = rd16(&entry[DIR_FstClusHI]);
                uint16_t lo = rd16(&entry[DIR_FstClusLO]);
                f->first_cluster    = ((uint32_t)hi << 16) | (uint32_t)lo;
                f->size             = rd32(&entry[DIR_FileSize]);
                f->dir_entry_lba    = abs_sector;
                f->dir_entry_offset = off;

                pr_info("[FAT32]   %s: %u bytes, cluster=%u\n",
                            f->name, f->size, f->first_cluster);
                file_count++;
            }
        }

        if (done) break;

        cluster = fat_next_cluster(cluster);
    }

    pr_info("[FAT32] Root directory scan complete: %d file(s)\n", file_count);
    return E_OK;
}

int fat32_init(uint32_t partition_lba)
{
    int i;

    pr_info("[FAT32] Initializing at partition LBA %u...\n", partition_lba);

    fs_bdev = get_gendisk("mmc0");
    if (!fs_bdev) {
        pr_err("[FAT32] ERROR: mmc0 block device not registered\n");
        return E_FAIL;
    }

    fs.part_lba = partition_lba;
    file_count = 0;
    for (i = 0; i < FAT32_MAX_FILES; i++) {
        file_table[i].in_use = false;
    }

    if (parse_bpb() != E_OK) {
        return E_FAIL;
    }

    if (scan_root_dir() != E_OK) {
        return E_FAIL;
    }

    return E_OK;
}

/* ============================================================
 * VFS Operations — Read Path
 * ============================================================ */

struct fat32_resolve {
    uint32_t cluster;
    uint32_t size;
    bool     is_dir;
    uint32_t entry_lba;
    uint32_t entry_off;
    uint8_t  raw_name[11];
    uint8_t  nt_res;
};

/* Scan a single directory (identified by first_cluster) for an entry
 * whose 11-byte raw name matches `query`. Returns E_OK on hit, E_NOENT
 * on exhaustion, E_FAIL on I/O error. */
static int dir_lookup_component(uint32_t dir_cluster, const uint8_t query[11],
                                 struct fat32_resolve *out)
{
    uint32_t cluster = dir_cluster;

    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint32_t base_sector = cluster_to_sector(cluster);

        for (uint32_t s = 0; s < fs.sectors_per_cluster; s++) {
            uint32_t abs_sector = base_sector + s;
            struct buffer_head *bh = bread(fs_bdev, abs_sector);
            if (!bh) return E_FAIL;

            for (uint32_t off = 0; off < FAT32_SECTOR_SZ; off += FAT32_DIR_ENTRY_SZ) {
                uint8_t *entry = &bh->data[off];
                uint8_t first  = entry[DIR_Name];
                uint8_t attr   = entry[DIR_Attr];

                if (first == DIR_ENTRY_END) { brelse(bh); return E_NOENT; }
                if (first == DIR_ENTRY_DELETED) continue;
                if (attr == ATTR_LONG_NAME)     continue;
                if (attr & ATTR_VOLUME_ID)      continue;

                if (raw_name_eq(&entry[DIR_Name], query)) {
                    uint16_t hi = rd16(&entry[DIR_FstClusHI]);
                    uint16_t lo = rd16(&entry[DIR_FstClusLO]);
                    out->cluster   = ((uint32_t)hi << 16) | (uint32_t)lo;
                    out->size      = rd32(&entry[DIR_FileSize]);
                    out->is_dir    = (attr & ATTR_DIRECTORY) != 0;
                    out->entry_lba = abs_sector;
                    out->entry_off = off;
                    for (int k = 0; k < 11; k++) out->raw_name[k] = entry[DIR_Name + k];
                    out->nt_res = entry[DIR_NTRes];
                    brelse(bh);
                    return E_OK;
                }
            }
            brelse(bh);
        }

        cluster = fat_next_cluster(cluster);
    }
    return E_NOENT;
}

/* Walks a slash-separated path component-by-component starting from
 * the root directory, descending through subdirectories until the
 * tail is resolved (or missing). */
static int resolve_path(const char *path, struct fat32_resolve *out)
{
    while (*path == '/') path++;

    if (*path == '\0') {
        /* Root */
        out->cluster   = fs.root_cluster;
        out->size      = 0;
        out->is_dir    = true;
        out->entry_lba = 0;
        out->entry_off = 0;
        return E_OK;
    }

    uint32_t cur_cluster = fs.root_cluster;
    bool     cur_is_dir  = true;

    while (*path) {
        if (!cur_is_dir) return E_NOENT;

        /* Extract one component. */
        const char *sep = path;
        while (*sep && *sep != '/') sep++;

        /* FAT32 MVP: 8.3 only. Copy component into a small buffer and
         * build the raw 11-byte form. */
        char comp[13];
        uint32_t clen = (uint32_t)(sep - path);
        if (clen > 12) clen = 12;
        for (uint32_t i = 0; i < clen; i++) comp[i] = path[i];
        comp[clen] = '\0';

        uint8_t query[11];
        name_to_raw(comp, query);

        int rc = dir_lookup_component(cur_cluster, query, out);
        if (rc != E_OK) return rc;

        cur_cluster = out->cluster;
        cur_is_dir  = out->is_dir;

        path = sep;
        while (*path == '/') path++;
    }

    return E_OK;
}

/* Cache the result of a path resolution in file_table so read/write
 * can look up by index. De-dupes by (entry_lba, entry_off) — reopens
 * of the same file return the same slot. */
static int cache_file(const struct fat32_resolve *r)
{
    for (int i = 0; i < file_count; i++) {
        if (file_table[i].in_use &&
            file_table[i].dir_entry_lba == r->entry_lba &&
            file_table[i].dir_entry_offset == r->entry_off) {
            /* Refresh — size may have grown since last open. */
            file_table[i].first_cluster = r->cluster;
            file_table[i].size          = r->size;
            return i;
        }
    }

    if (file_count >= FAT32_MAX_FILES) {
        pr_info("[FAT32] WARNING: file_table full\n");
        return E_MFILE;
    }

    struct fat32_file *f = &file_table[file_count];
    f->in_use = true;
    for (int k = 0; k < 11; k++) f->raw_name[k] = r->raw_name[k];
    f->nt_res           = r->nt_res;
    raw_to_name(f->raw_name, f->nt_res, f->name);
    f->first_cluster    = r->cluster;
    f->size             = r->size;
    f->dir_entry_lba    = r->entry_lba;
    f->dir_entry_offset = r->entry_off;
    return file_count++;
}

static int fat32_lookup(const char *path)
{
    struct fat32_resolve r;
    int rc = resolve_path(path, &r);
    if (rc != E_OK) return rc;
    if (r.is_dir) return E_NOENT;  /* vfs_open on dirs isn't supported */
    return cache_file(&r);
}

static uint32_t walk_chain(uint32_t first_cluster, uint32_t cluster_skip)
{
    uint32_t cur = first_cluster;
    uint32_t i;

    for (i = 0; i < cluster_skip; i++) {
        cur = fat_next_cluster(cur);
        if (cur >= FAT32_EOC || cur == 0) {
            return 0;
        }
    }
    return cur;
}

static int fat32_read(int file_index, uint32_t offset, void *buf, uint32_t len)
{
    if (file_index < 0 || file_index >= file_count) {
        return E_BADF;
    }

    struct fat32_file *f = &file_table[file_index];

    if (offset >= f->size) {
        return 0;
    }

    uint32_t remaining = fat32_min(len, f->size - offset);
    uint32_t written = 0;
    uint8_t *out = (uint8_t *)buf;

    uint32_t cluster_skip = offset / fs.bytes_per_cluster;
    uint32_t cur_cluster = walk_chain(f->first_cluster, cluster_skip);
    if (cur_cluster == 0) {
        return E_FAIL;
    }

    uint32_t intra_cluster = offset % fs.bytes_per_cluster;

    while (remaining > 0) {
        uint32_t base_sector = cluster_to_sector(cur_cluster);
        uint32_t sec_in_clus = intra_cluster / fs.bytes_per_sector;
        uint32_t intra_sec   = intra_cluster % fs.bytes_per_sector;

        while (sec_in_clus < fs.sectors_per_cluster && remaining > 0) {
            struct buffer_head *bh = bread(fs_bdev, base_sector + sec_in_clus);
            if (!bh) return E_FAIL;

            uint32_t chunk = fat32_min(remaining, FAT32_SECTOR_SZ - intra_sec);
            memcpy(out + written, bh->data + intra_sec, chunk);
            brelse(bh);

            written   += chunk;
            remaining -= chunk;
            intra_sec  = 0;
            sec_in_clus++;
        }

        if (remaining == 0) break;

        cur_cluster = fat_next_cluster(cur_cluster);
        if (cur_cluster >= FAT32_EOC || cur_cluster == 0) {
            break;
        }
        intra_cluster = 0;
    }

    return (int)written;
}

static int fat32_get_file_count(void)
{
    return file_count;
}

static int fat32_get_file_info(int index, char *name_out, uint32_t *size_out)
{
    if (index < 0 || index >= file_count) {
        return E_BADF;
    }

    struct fat32_file *f = &file_table[index];
    strcpy(name_out, f->name);
    *size_out = f->size;

    return E_OK;
}

/* Enumerate entries in a directory located by fs-relative `path`.
 * Root is "", everything else walks subdirectories. */
static int fat32_listdir(const char *path, void *entries, uint32_t max)
{
    file_info_t *out = (file_info_t *)entries;
    uint32_t dir_cluster;

    if (!path || *path == '\0') {
        dir_cluster = fs.root_cluster;
    } else {
        struct fat32_resolve r;
        int rc = resolve_path(path, &r);
        if (rc != E_OK) return rc;
        if (!r.is_dir) return E_NOENT;
        dir_cluster = r.cluster;
    }

    uint32_t count = 0;
    uint32_t cluster = dir_cluster;
    bool     done    = false;

    while (!done && cluster >= 2 && cluster < FAT32_EOC && count < max) {
        uint32_t base_sector = cluster_to_sector(cluster);

        for (uint32_t s = 0; s < fs.sectors_per_cluster && !done && count < max; s++) {
            uint32_t abs_sector = base_sector + s;
            struct buffer_head *bh = bread(fs_bdev, abs_sector);
            if (!bh) return E_FAIL;

            for (uint32_t off = 0;
                 off < FAT32_SECTOR_SZ && count < max;
                 off += FAT32_DIR_ENTRY_SZ) {

                uint8_t *entry = &bh->data[off];
                uint8_t first  = entry[DIR_Name];
                uint8_t attr   = entry[DIR_Attr];

                if (first == DIR_ENTRY_END)    { done = true; break; }
                if (first == DIR_ENTRY_DELETED) continue;
                if (attr == ATTR_LONG_NAME)     continue;
                if (attr & ATTR_VOLUME_ID)      continue;
                if (first == '.')               continue;  /* skip . and .. */

                raw_to_name(&entry[DIR_Name], entry[DIR_NTRes], out[count].name);
                out[count].size = (attr & ATTR_DIRECTORY) ? 0
                                                          : rd32(&entry[DIR_FileSize]);
                count++;
            }
            brelse(bh);
        }

        if (done) break;
        cluster = fat_next_cluster(cluster);
    }

    return (int)count;
}

/* ============================================================
 * Write Path
 * ============================================================ */

/* Writes must land in ALL num_fats copies, otherwise host OSes
 * detect mismatch on next mount and "repair" unpredictably. */
static int fat_set_entry(uint32_t cluster, uint32_t value)
{
    uint32_t fat_byte_off   = cluster * 4;
    uint32_t fat_sector_idx = fat_byte_off / FAT32_SECTOR_SZ;
    uint32_t sec_off        = fat_byte_off % FAT32_SECTOR_SZ;

    for (uint32_t f = 0; f < fs.num_fats; f++) {
        uint32_t abs_sector = fs.fat_lba + f * fs.fat_sectors + fat_sector_idx;

        struct buffer_head *bh = bread(fs_bdev, abs_sector);
        if (!bh) return E_FAIL;

        /* Preserve the top 4 reserved bits per FAT32 spec */
        uint32_t old = rd32(&bh->data[sec_off]);
        uint32_t new_val = (old & 0xF0000000U) | (value & 0x0FFFFFFFU);

        bh->data[sec_off + 0] = (uint8_t)(new_val);
        bh->data[sec_off + 1] = (uint8_t)(new_val >> 8);
        bh->data[sec_off + 2] = (uint8_t)(new_val >> 16);
        bh->data[sec_off + 3] = (uint8_t)(new_val >> 24);
        bmark_dirty(bh);
        brelse(bh);
    }
    return E_OK;
}

/* Linear scan — no free-cluster hint yet. O(N) worst case. */
static uint32_t fat_alloc_cluster(void)
{
    uint32_t max_entries = fs.fat_sectors * (FAT32_SECTOR_SZ / 4);
    uint32_t c;

    for (c = 2; c < max_entries; c++) {
        uint32_t entry = fat_next_cluster(c);
        if (entry == 0) {
            if (fat_set_entry(c, FAT32_EOC) != E_OK) {
                return 0;
            }
            return c;
        }
    }
    return 0;
}

static int fat_free_chain(uint32_t start)
{
    uint32_t cur = start;

    while (cur >= 2 && cur < FAT32_EOC) {
        uint32_t next = fat_next_cluster(cur);
        if (fat_set_entry(cur, 0) != E_OK) {
            return E_FAIL;
        }
        cur = next;
    }
    return E_OK;
}

/* Flush in-memory file size and first cluster back to the directory
 * entry on disk. Skipping this leaks clusters on reboot. */
static int update_dir_entry(const struct fat32_file *f)
{
    struct buffer_head *bh = bread(fs_bdev, f->dir_entry_lba);
    if (!bh) return E_FAIL;

    uint8_t *entry = &bh->data[f->dir_entry_offset];

    uint16_t hi = (uint16_t)(f->first_cluster >> 16);
    uint16_t lo = (uint16_t)(f->first_cluster & 0xFFFF);
    entry[DIR_FstClusHI + 0] = (uint8_t)(hi);
    entry[DIR_FstClusHI + 1] = (uint8_t)(hi >> 8);
    entry[DIR_FstClusLO + 0] = (uint8_t)(lo);
    entry[DIR_FstClusLO + 1] = (uint8_t)(lo >> 8);

    entry[DIR_FileSize + 0] = (uint8_t)(f->size);
    entry[DIR_FileSize + 1] = (uint8_t)(f->size >> 8);
    entry[DIR_FileSize + 2] = (uint8_t)(f->size >> 16);
    entry[DIR_FileSize + 3] = (uint8_t)(f->size >> 24);

    bmark_dirty(bh);
    brelse(bh);
    return E_OK;
}

static int fat32_write(int file_index, uint32_t offset, const void *buf, uint32_t len)
{
    if (file_index < 0 || file_index >= file_count) {
        return E_BADF;
    }
    if (len == 0) {
        return 0;
    }

    struct fat32_file *f = &file_table[file_index];

    /* Empty file needs its first cluster */
    if (f->first_cluster == 0) {
        uint32_t new_c = fat_alloc_cluster();
        if (new_c == 0) {
            pr_err("[FAT32] ERROR: out of space (alloc first cluster)\n");
            return E_FAIL;
        }
        f->first_cluster = new_c;
    }

    /* Walk to the cluster containing `offset`, extending chain if needed */
    uint32_t cluster_skip = offset / fs.bytes_per_cluster;
    uint32_t cur = f->first_cluster;
    uint32_t i;

    for (i = 0; i < cluster_skip; i++) {
        uint32_t next = fat_next_cluster(cur);
        if (next >= FAT32_EOC || next == 0) {
            uint32_t new_c = fat_alloc_cluster();
            if (new_c == 0) return E_FAIL;
            if (fat_set_entry(cur, new_c) != E_OK) return E_FAIL;
            next = new_c;
        }
        cur = next;
    }

    uint32_t intra_cluster = offset % fs.bytes_per_cluster;
    uint32_t remaining = len;
    uint32_t written = 0;
    const uint8_t *in = (const uint8_t *)buf;

    while (remaining > 0) {
        uint32_t base_sector  = cluster_to_sector(cur);
        uint32_t sec_in_clus  = intra_cluster / fs.bytes_per_sector;
        uint32_t intra_sec    = intra_cluster % fs.bytes_per_sector;

        while (sec_in_clus < fs.sectors_per_cluster && remaining > 0) {
            uint32_t abs_sector = base_sector + sec_in_clus;
            uint32_t chunk = fat32_min(remaining, FAT32_SECTOR_SZ - intra_sec);

            struct buffer_head *bh = bread(fs_bdev, abs_sector);
            if (!bh) return E_FAIL;
            memcpy(bh->data + intra_sec, in + written, chunk);
            bmark_dirty(bh);
            brelse(bh);

            written   += chunk;
            remaining -= chunk;
            intra_sec  = 0;
            sec_in_clus++;
        }

        if (remaining == 0) break;

        uint32_t next = fat_next_cluster(cur);
        if (next >= FAT32_EOC || next == 0) {
            uint32_t new_c = fat_alloc_cluster();
            if (new_c == 0) break;      /* Out of space — return partial */
            if (fat_set_entry(cur, new_c) != E_OK) return E_FAIL;
            next = new_c;
        }
        cur = next;
        intra_cluster = 0;
    }

    uint32_t new_end = offset + written;
    if (new_end > f->size) {
        f->size = new_end;
    }

    if (update_dir_entry(f) != E_OK) {
        pr_info("[FAT32] WARNING: failed to update dir entry for %s\n", f->name);
    }

    return (int)written;
}

/* Mark base or ext lowercase if the user-supplied name is consistently
 * lowercase in that part. Mixed case falls back to uppercase on disk. */
static uint8_t detect_nt_res(const char *name)
{
    uint8_t res = 0;
    bool base_has_lower = false, base_has_upper = false;
    bool ext_has_lower = false,  ext_has_upper = false;
    const char *p = name;

    while (*p && *p != '.') {
        if (*p >= 'a' && *p <= 'z') base_has_lower = true;
        if (*p >= 'A' && *p <= 'Z') base_has_upper = true;
        p++;
    }
    if (*p == '.') p++;
    while (*p) {
        if (*p >= 'a' && *p <= 'z') ext_has_lower = true;
        if (*p >= 'A' && *p <= 'Z') ext_has_upper = true;
        p++;
    }

    if (base_has_lower && !base_has_upper) res |= NT_RES_LOWER_BASE;
    if (ext_has_lower  && !ext_has_upper)  res |= NT_RES_LOWER_EXT;
    return res;
}

static int fat32_create(const char *name)
{
    if (file_count >= FAT32_MAX_FILES) {
        pr_err("[FAT32] ERROR: file_table full\n");
        return E_FAIL;
    }

    uint8_t raw[11];
    name_to_raw(name, raw);
    uint8_t nt_res = detect_nt_res(name);

    uint32_t cluster = fs.root_cluster;

    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint32_t base_sector = cluster_to_sector(cluster);
        uint32_t s;

        for (s = 0; s < fs.sectors_per_cluster; s++) {
            uint32_t abs_sector = base_sector + s;

            struct buffer_head *bh = bread(fs_bdev, abs_sector);
            if (!bh) return E_FAIL;

            uint32_t off;
            for (off = 0; off < FAT32_SECTOR_SZ; off += FAT32_DIR_ENTRY_SZ) {
                uint8_t first = bh->data[off];
                if (first != DIR_ENTRY_END && first != DIR_ENTRY_DELETED) {
                    continue;
                }

                uint8_t *entry = &bh->data[off];
                int k;

                for (k = 0; k < FAT32_DIR_ENTRY_SZ; k++) entry[k] = 0;
                for (k = 0; k < 11; k++) entry[DIR_Name + k] = raw[k];
                entry[DIR_Attr]  = ATTR_ARCHIVE;
                entry[DIR_NTRes] = nt_res;

                /* Consuming an END marker — push it forward so the chain
                 * terminator stays after the newly created entry. */
                if (first == DIR_ENTRY_END && (off + FAT32_DIR_ENTRY_SZ) < FAT32_SECTOR_SZ) {
                    bh->data[off + FAT32_DIR_ENTRY_SZ] = DIR_ENTRY_END;
                }

                bmark_dirty(bh);
                brelse(bh);

                struct fat32_file *f = &file_table[file_count];
                f->in_use = true;
                for (k = 0; k < 11; k++) f->raw_name[k] = raw[k];
                f->nt_res = nt_res;
                raw_to_name(raw, nt_res, f->name);
                f->first_cluster    = 0;
                f->size             = 0;
                f->dir_entry_lba    = abs_sector;
                f->dir_entry_offset = off;

                int new_idx = file_count;
                file_count++;
                pr_info("[FAT32] Created file: %s (idx=%d)\n", f->name, new_idx);
                return new_idx;
            }
            brelse(bh);
        }

        cluster = fat_next_cluster(cluster);
    }

    /* Root directory cluster chain exhausted — growing root dir not supported */
    pr_err("[FAT32] ERROR: root directory full\n");
    return E_FAIL;
}

/* Only truncate-to-zero is supported (needed by O_TRUNC). */
static int fat32_truncate(int file_index, uint32_t new_size)
{
    if (file_index < 0 || file_index >= file_count) {
        return E_BADF;
    }
    if (new_size != 0) {
        return E_FAIL;
    }

    struct fat32_file *f = &file_table[file_index];

    if (f->first_cluster != 0) {
        if (fat_free_chain(f->first_cluster) != E_OK) {
            return E_FAIL;
        }
        f->first_cluster = 0;
    }
    f->size = 0;

    return update_dir_entry(f);
}

/* Free file's cluster chain, mark dir entry deleted, drop from
 * file_table. Refuses directories — FAT32 MVP has no subdir write. */
static int fat32_unlink(const char *path)
{
    struct fat32_resolve r;
    int rc = resolve_path(path, &r);
    if (rc != E_OK) return rc;
    if (r.is_dir) return E_PERM;

    if (r.cluster >= 2) {
        int fr = fat_free_chain(r.cluster);
        if (fr != E_OK) return fr;
    }

    struct buffer_head *bh = bread(fs_bdev, r.entry_lba);
    if (!bh) return E_FAIL;
    bh->data[r.entry_off + DIR_Name] = DIR_ENTRY_DELETED;
    bmark_dirty(bh);
    brelse(bh);

    for (int i = 0; i < file_count; i++) {
        if (file_table[i].in_use &&
            file_table[i].dir_entry_lba    == r.entry_lba &&
            file_table[i].dir_entry_offset == r.entry_off) {
            file_table[i].in_use = false;
            break;
        }
    }
    return E_OK;
}

/* Rename within the same directory — overwrite the name / NTRes
 * fields of the existing dir entry. Cross-directory mv would need
 * entry allocation in the destination plus deletion of the source;
 * kept for a later iteration. */
static int fat32_rename(const char *old_path, const char *new_path)
{
    struct fat32_resolve src;
    int rc = resolve_path(old_path, &src);
    if (rc != E_OK) return rc;
    if (src.is_dir) return E_PERM;

    /* Reject if destination already exists so we never silently
     * clobber. */
    struct fat32_resolve dst_check;
    if (resolve_path(new_path, &dst_check) == E_OK) return E_PERM;

    uint8_t new_raw[11];
    name_to_raw(new_path, new_raw);
    uint8_t new_nt_res = detect_nt_res(new_path);

    struct buffer_head *bh = bread(fs_bdev, src.entry_lba);
    if (!bh) return E_FAIL;
    for (int i = 0; i < 11; i++) bh->data[src.entry_off + DIR_Name + i] = new_raw[i];
    bh->data[src.entry_off + DIR_NTRes] = new_nt_res;
    bmark_dirty(bh);
    brelse(bh);

    for (int i = 0; i < file_count; i++) {
        if (file_table[i].in_use &&
            file_table[i].dir_entry_lba    == src.entry_lba &&
            file_table[i].dir_entry_offset == src.entry_off) {
            for (int k = 0; k < 11; k++) file_table[i].raw_name[k] = new_raw[k];
            file_table[i].nt_res = new_nt_res;
            raw_to_name(new_raw, new_nt_res, file_table[i].name);
            break;
        }
    }
    return E_OK;
}

static struct vfs_operations fat32_ops = {
    .lookup         = fat32_lookup,
    .read           = fat32_read,
    .get_file_count = fat32_get_file_count,
    .get_file_info  = fat32_get_file_info,
    .listdir        = fat32_listdir,
    .create         = fat32_create,
    .write          = fat32_write,
    .truncate       = fat32_truncate,
    .unlink         = fat32_unlink,
    .rename         = fat32_rename,
};

struct vfs_operations *fat32_get_operations(void)
{
    return &fat32_ops;
}
