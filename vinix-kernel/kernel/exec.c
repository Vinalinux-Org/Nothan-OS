/* ============================================================
 * exec.c
 * ------------------------------------------------------------
 * ELF32 loader for do_exec().
 * ============================================================ */

#include "proc.h"
#include "vfs.h"
#include "syscalls.h"
#include "uart.h"
#include "string.h"
#include "types.h"
#include "mmu.h"

#define ELF_MAGIC_0 0x7F
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'
#define ELFCLASS32  1
#define ELFDATA2LSB 1
#define ET_EXEC     2
#define EM_ARM      40
#define EV_CURRENT  1
#define PT_LOAD     1

#define EXEC_READ_CHUNK 128

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

static int in_user_space(uint32_t addr, uint32_t len)
{
    uint32_t end = addr + len;
    if (end < addr) return 0;
    uint32_t lo = USER_SPACE_VA;
    uint32_t hi = USER_SPACE_VA + (USER_SPACE_MB * 1024u * 1024u);
    return (addr >= lo && end <= hi) ? 1 : 0;
}

static int read_exact(int fd, void *buf, uint32_t len)
{
    uint8_t *dst = buf;
    uint32_t total = 0;
    while (total < len) {
        int n = vfs_read(fd, dst + total, len - total);
        if (n <= 0) return E_FAIL;
        total += (uint32_t)n;
    }
    return E_OK;
}

static int skip_bytes(int fd, uint32_t len)
{
    uint8_t tmp[EXEC_READ_CHUNK];
    uint32_t left = len;
    while (left > 0) {
        uint32_t chunk = (left > EXEC_READ_CHUNK) ? EXEC_READ_CHUNK : left;
        int n = vfs_read(fd, tmp, chunk);
        if (n <= 0) return E_FAIL;
        left -= (uint32_t)n;
    }
    return E_OK;
}

static int validate_header(const elf32_ehdr_t *h)
{
    if (h->e_ident[0] != ELF_MAGIC_0 || h->e_ident[1] != ELF_MAGIC_1 ||
        h->e_ident[2] != ELF_MAGIC_2 || h->e_ident[3] != ELF_MAGIC_3)
        return E_ARG;

    if (h->e_ident[4] != ELFCLASS32 || h->e_ident[5] != ELFDATA2LSB)
        return E_ARG;

    if (h->e_type != ET_EXEC || h->e_machine != EM_ARM || h->e_version != EV_CURRENT)
        return E_ARG;

    if (h->e_phentsize != sizeof(elf32_phdr_t) || h->e_phnum == 0)
        return E_ARG;

    if (h->e_phoff != sizeof(elf32_ehdr_t))
        return E_ARG;

    if (!in_user_space(h->e_entry, 4))
        return E_PTR;

    return E_OK;
}

/* argv staging area — top of user space, above initial SP so the
 * user stack never collides with it. See userspace linker layout.
 *   [USER_ARGV_AREA_VA .. USER_STACK_BASE) = 512 bytes reserved:
 *     argv[] pointer array (≤16 entries × 4 B)  — first 64 bytes
 *     argument strings packed tail-to-head      — remaining 448 B
 */
#define USER_ARGV_MAX      16u
#define USER_ARGV_AREA_SZ  512u
#define USER_ARGV_AREA_VA  (USER_SPACE_VA + USER_SPACE_MB * 1024u * 1024u - USER_ARGV_AREA_SZ)
#define USER_ARGV_STRS_SZ  (USER_ARGV_AREA_SZ - USER_ARGV_MAX * sizeof(char *))

/* Snapshot argv strings into a kernel buffer before the ELF load
 * wipes user memory. Returns argc (0..USER_ARGV_MAX) or E_* on bad
 * pointers. kbuf must be at least USER_ARGV_STRS_SZ bytes. */
static int snapshot_argv(char **argv_user, char *kbuf, uint16_t *arg_offsets)
{
    if (!argv_user) return 0;
    if (!in_user_space((uint32_t)argv_user, sizeof(char *) * USER_ARGV_MAX))
        return E_PTR;

    uint32_t argc = 0;
    uint32_t pos  = 0;

    while (argc < USER_ARGV_MAX) {
        char *s = argv_user[argc];
        if (!s) break;
        if (!in_user_space((uint32_t)s, 1)) return E_PTR;

        arg_offsets[argc] = (uint16_t)pos;
        /* Copy the string, bounded by kbuf size. */
        while (*s) {
            if (!in_user_space((uint32_t)s, 1)) return E_PTR;
            if (pos + 1 >= USER_ARGV_STRS_SZ) return E_ARG;  /* too long */
            kbuf[pos++] = *s++;
        }
        kbuf[pos++] = '\0';
        argc++;
    }
    return (int)argc;
}

/* After ELF load, write argv back into the fresh user memory at the
 * top-of-space argv area. Sets ctx->r0 = argc, ctx->r1 = user VA of
 * the argv[] array. */
static void install_argv(struct svc_context *ctx, int argc,
                         const char *kbuf, const uint16_t *arg_offsets,
                         uint32_t snap_pos)
{
    if (argc <= 0) {
        ctx->r0 = 0;
        ctx->r1 = 0;
        return;
    }

    char **argv_arr = (char **)USER_ARGV_AREA_VA;
    char  *strs     = (char *)(USER_ARGV_AREA_VA + USER_ARGV_MAX * sizeof(char *));

    memcpy(strs, kbuf, snap_pos);
    for (int i = 0; i < argc; i++) {
        argv_arr[i] = strs + arg_offsets[i];
    }
    argv_arr[argc] = 0;  /* NULL terminator */

    ctx->r0 = (uint32_t)argc;
    ctx->r1 = (uint32_t)argv_arr;
}

int do_exec(const char *path, struct svc_context *ctx, char **argv_user)
{
    elf32_ehdr_t ehdr;
    elf32_phdr_t phdr;
    int fd;

    /* Snapshot argv before we trample user memory. */
    char     argv_kbuf[USER_ARGV_STRS_SZ];
    uint16_t arg_offsets[USER_ARGV_MAX];
    int argc = snapshot_argv(argv_user, argv_kbuf, arg_offsets);
    if (argc < 0) return argc;
    uint32_t snap_pos = 0;
    for (int i = 0; i < argc; i++) {
        snap_pos = arg_offsets[i];
        while (argv_kbuf[snap_pos]) snap_pos++;
        snap_pos++;  /* past null terminator */
    }

    fd = vfs_open(path, O_RDONLY);
    if (fd < 0) return fd;

    if (read_exact(fd, &ehdr, sizeof(ehdr)) != E_OK) {
        vfs_close(fd);
        return E_FAIL;
    }

    int ok = validate_header(&ehdr);
    if (ok != E_OK) {
        vfs_close(fd);
        return ok;
    }

    for (uint32_t i = 0; i < ehdr.e_phnum; i++) {
        if (read_exact(fd, &phdr, sizeof(phdr)) != E_OK) {
            vfs_close(fd);
            return E_FAIL;
        }

        if (phdr.p_type != PT_LOAD) continue;

        if (phdr.p_filesz > phdr.p_memsz) {
            vfs_close(fd);
            return E_ARG;
        }

        if (!in_user_space(phdr.p_vaddr, phdr.p_memsz)) {
            vfs_close(fd);
            return E_PTR;
        }

        int seg = vfs_open(path, O_RDONLY);
        if (seg < 0) {
            vfs_close(fd);
            return seg;
        }
        if (skip_bytes(seg, phdr.p_offset) != E_OK ||
            read_exact(seg, (void *)phdr.p_vaddr, phdr.p_filesz) != E_OK) {
            vfs_close(seg);
            vfs_close(fd);
            return E_FAIL;
        }
        vfs_close(seg);

        if (phdr.p_memsz > phdr.p_filesz) {
            memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0,
                   phdr.p_memsz - phdr.p_filesz);
        }
    }

    vfs_close(fd);

    install_argv(ctx, argc, argv_kbuf, arg_offsets, snap_pos);
    ctx->r2 = 0;
    ctx->r3 = 0;
    ctx->lr = ehdr.e_entry;
    return E_OK;
}
