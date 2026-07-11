/*
 * storage_daemon.c - FAT-write backend for NothanOS
 *
 * Takes storage_write() requests off the GUI task, so a slow FAT/SD write
 * never blocks touch input. GUI hands a (path, blob) pair over the
 * storage bus (/dev/storage_fe -> /dev/storage_be, storagebus.c); this
 * daemon does the actual blocking open()+writefile()+close() on its own
 * task, whenever the SD card gets around to it.
 *
 * Wire format (raw bytes, one frame per save, no ack — fire and forget):
 *   [magic0][magic1][path_len:u16 LE][data_len:u32 LE][path bytes][data bytes]
 *
 * Single-owner, non-blocking read + cooperative yield() — same style as
 * phone_daemon.c's frame assembler, just one direction and one message
 * shape instead of framed JSON.
 */

#include "../lib/syscall.h"
#include "../lib/printf.h"
#include "../lib/string.h"

#define STORAGE_DEV   "/dev/storage_be"
#define STORAGE_MAGIC0 0xB5
#define STORAGE_MAGIC1 0x0B

#define PATH_MAX      64
#define DATA_MAX      (600 * 1024)   /* covers the largest caller: the SMS store */
#define READ_CHUNK    4096

enum { ST_MAGIC0, ST_MAGIC1, ST_HDR, ST_PATH, ST_DATA };

static int           g_state;
static unsigned char g_hdr[6];        /* path_len(2 LE) + data_len(4 LE) */
static int           g_hdr_have;
static int           g_path_len;
static char          g_path[PATH_MAX];
static int           g_path_have;
static unsigned int  g_data_len;
static int           g_data_have;
static char          g_data[DATA_MAX];

static void reset_frame(void)
{
    g_state    = ST_MAGIC0;
    g_hdr_have = 0;
}

static void do_save(void)
{
    long fd = open(g_path, O_WRONLY | O_CREAT);
    if (fd < 0) {
        printf("[sd] open failed: %s\n", g_path);
        return;
    }
    long n = writefile(fd, g_data, g_data_len);
    close(fd);
    printf("[sd] saved %s (%d/%u bytes)\n", g_path, (int)n, g_data_len);
}

/* Feed one byte through the frame assembler. */
static void feed_byte(unsigned char b)
{
    switch (g_state) {
    case ST_MAGIC0:
        if (b == STORAGE_MAGIC0) {
            g_state = ST_MAGIC1;
        }
        break;

    case ST_MAGIC1:
        if (b == STORAGE_MAGIC1) {
            g_state    = ST_HDR;
            g_hdr_have = 0;
        } else {
            g_state = ST_MAGIC0;   /* resync */
        }
        break;

    case ST_HDR:
        g_hdr[g_hdr_have++] = b;
        if (g_hdr_have == (int)sizeof(g_hdr)) {
            g_path_len = (int)((unsigned)g_hdr[0] | ((unsigned)g_hdr[1] << 8));
            g_data_len = (unsigned)g_hdr[2]
                       | ((unsigned)g_hdr[3] << 8)
                       | ((unsigned)g_hdr[4] << 16)
                       | ((unsigned)g_hdr[5] << 24);
            if (g_path_len <= 0 || g_path_len >= PATH_MAX || g_data_len > DATA_MAX) {
                printf("[sd] bad frame header (path_len=%d data_len=%u) — resync\n",
                       g_path_len, g_data_len);
                reset_frame();
                break;
            }
            g_path_have = 0;
            g_state     = ST_PATH;
        }
        break;

    case ST_PATH:
        g_path[g_path_have++] = (char)b;
        if (g_path_have == g_path_len) {
            g_path[g_path_have] = '\0';
            if (g_data_len == 0) {
                do_save();
                reset_frame();
            } else {
                g_data_have = 0;
                g_state     = ST_DATA;
            }
        }
        break;

    case ST_DATA:
        g_data[g_data_have++] = (char)b;
        if (g_data_have == (int)g_data_len) {
            do_save();
            reset_frame();
        }
        break;
    }
}

int main(void)
{
    printf("[sd] starting\n");

    long fd = open(STORAGE_DEV, O_RDONLY);
    if (fd < 0) {
        printf("[sd] cannot open %s\n", STORAGE_DEV);
        return 1;
    }
    printf("[sd] %s opened\n", STORAGE_DEV);

    reset_frame();

    static unsigned char chunk[READ_CHUNK];
    for (;;) {
        long n = read((int)fd, chunk, sizeof(chunk));
        if (n > 0) {
            for (long i = 0; i < n; i++) {
                feed_byte(chunk[i]);
            }
        } else {
            yield();
        }
    }
}
