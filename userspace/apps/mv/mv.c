/* mv — rename a file on the same FAT32 mount */

#include "stdio.h"
#include "unistd.h"

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("Usage: mv <src> <dst>\n");
        return 1;
    }
    if (rename(argv[1], argv[2]) < 0) {
        printf("mv: cannot rename '%s' -> '%s'\n", argv[1], argv[2]);
        return 1;
    }
    return 0;
}
