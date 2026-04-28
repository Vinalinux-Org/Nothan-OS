/* free — print memory stats by streaming /proc/meminfo */

#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
        printf("free: /proc/meminfo: open failed\n");
        return 1;
    }

    char buf[256];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, (size_t)n);
    }
    close(fd);
    return 0;
}
