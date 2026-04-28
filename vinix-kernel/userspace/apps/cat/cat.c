/* cat — dump file(s) to stdout */

#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"

static int dump(int fd)
{
    char buf[256];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, (size_t)n);
    }
    return n < 0 ? 1 : 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        /* Stream stdin to stdout when invoked without args. */
        return dump(0);
    }

    int rc = 0;
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            printf("cat: %s: open failed\n", argv[i]);
            rc = 1;
            continue;
        }
        if (dump(fd) != 0) rc = 1;
        close(fd);
    }
    return rc;
}
