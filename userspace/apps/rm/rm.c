/* rm — unlink file on FAT32 root */

#include "stdio.h"
#include "unistd.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return 1;
    }

    int rc = 0;
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            printf("rm: %s: cannot remove\n", argv[i]);
            rc = 1;
        }
    }
    return rc;
}
