/*
 * phone_frame.c — frame encode/decode + CRC16-CCITT
 *
 * Frame: [0xAA][0x55][LEN_LO][LEN_HI][JSON...][CRC_LO][CRC_HI]
 * CRC16-CCITT (poly 0x1021, init 0xFFFF) over the JSON payload.
 */

#include "phone_frame.h"
#include "../lib/types.h"
#include "../lib/string.h"
#include "../lib/syscall.h"

#ifndef NULL
#  define NULL ((void *)0)
#endif
#ifndef EINTR
#  define EINTR 4
#endif
/* stub: the blocking I/O helpers reference errno for EINTR loops, but none
 * of our builds call them (daemon + GUI both use a non-blocking assembler).
 * A plain define avoids dragging in an external symbol that collides with
 * the daemon's own `int errno` (in pd_port.h). */
#define errno 0
#define write(fd, buf, len)  writefile((fd), (buf), (len))

/* CRC16-CCITT */
uint16_t phone_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* Encode */
int phone_frame_encode(uint8_t *frame, size_t frame_size,
                       const char *json, size_t json_len)
{
    /* header(4) + payload + crc(2) */
    size_t total = 4 + json_len + 2;
    if (total > frame_size || json_len > PHONE_JSON_MAX) {
        return -1;
    }

    frame[0] = PHONE_MAGIC0;
    frame[1] = PHONE_MAGIC1;
    frame[2] = (uint8_t)(json_len & 0xFF);
    frame[3] = (uint8_t)((json_len >> 8) & 0xFF);

    memcpy(frame + 4, json, json_len);

    uint16_t crc = phone_crc16((const uint8_t *)json, json_len);
    frame[4 + json_len]     = (uint8_t)(crc & 0xFF);
    frame[4 + json_len + 1] = (uint8_t)((crc >> 8) & 0xFF);

    return (int)total;
}

/* Decode */
int phone_frame_decode(const uint8_t *frame, size_t frame_size,
                       char *json, size_t json_size)
{
    if (frame_size < 6) {
        return -1;
    }
    if (frame[0] != PHONE_MAGIC0 || frame[1] != PHONE_MAGIC1) {
        return -1;
    }

    uint16_t len = (uint16_t)frame[2] | ((uint16_t)frame[3] << 8);
    if ((size_t)(4 + len + 2) > frame_size || len >= json_size) {
        return -1;
    }

    uint16_t got_crc = (uint16_t)frame[4 + len] |
                       ((uint16_t)frame[4 + len + 1] << 8);
    uint16_t exp_crc = phone_crc16(frame + 4, len);
    if (got_crc != exp_crc) {
        return -1;
    }

    memcpy(json, frame + 4, len);
    json[len] = '\0';
    return (int)len;
}

/* I/O helpers */

/* Read exactly n bytes from fd (blocking — fd must be opened without O_NONBLOCK). */
static int read_exact(int fd, uint8_t *buf, size_t n)
{
    size_t done = 0;
    while (done < n) {
        ssize_t r = read(fd, buf + done, n - done);
        if (r > 0) {
            done += (size_t)r;
            continue;
        }
        if (r < 0 && errno == EINTR) {
            continue;
        }
        return -1;
    }
    return 0;
}

/* Blocking — fd must be opened without O_NONBLOCK. */
int phone_frame_read(int fd, char *json, size_t json_size)
{
    uint8_t b;

    /* Sync: scan for 0xAA then 0x55. If the byte after 0xAA is also
     * 0xAA, treat it as a new magic0 candidate rather than discarding it
     * (handles noise patterns like 0xAA 0xAA 0x55 without losing sync). */
    for (;;) {
        if (read_exact(fd, &b, 1) < 0) {
            return -1;
        }
        if (b != PHONE_MAGIC0) {
            continue;
        }
again_magic1:
        if (read_exact(fd, &b, 1) < 0) {
            return -1;
        }
        if (b == PHONE_MAGIC1) {
            break;
        }
        if (b == PHONE_MAGIC0) {
            goto again_magic1;
        }
    }

    /* Read 2-byte length LE */
    uint8_t lbuf[2];
    if (read_exact(fd, lbuf, 2) < 0) {
        return -1;
    }
    uint16_t len = (uint16_t)lbuf[0] | ((uint16_t)lbuf[1] << 8);

    if (len == 0 || len > PHONE_JSON_MAX || (size_t)len >= json_size) {
        return -1;
    }

    /* Read payload + CRC */
    uint8_t tmp[PHONE_JSON_MAX + 2];
    if (read_exact(fd, tmp, len + 2) < 0) {
        return -1;
    }

    uint16_t got_crc = (uint16_t)tmp[len] | ((uint16_t)tmp[len + 1] << 8);
    uint16_t exp_crc = phone_crc16(tmp, len);
    if (got_crc != exp_crc) {
        return -1;
    }

    memcpy(json, tmp, len);
    json[len] = '\0';
    return (int)len;
}

int phone_frame_write(int fd, const char *json, size_t json_len)
{
    uint8_t frame[PHONE_FRAME_MAX];
    size_t  done = 0;
    int total = phone_frame_encode(frame, sizeof(frame), json, json_len);
    if (total < 0) {
        return -1;
    }
    while (done < (size_t)total) {
        ssize_t w = write(fd, frame + done, (size_t)total - done);
        if (w > 0) {
            done += (size_t)w;
            continue;
        }
        if (w < 0 && errno == EINTR) {
            continue;
        }
        return -1;
    }
    return 0;
}
