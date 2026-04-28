/* ls — list directory contents */

#include "stdio.h"
#include "syscalls.h"
#include "user_syscall.h"

int main(int argc, char **argv)
{
    const char *path = (argc >= 2) ? argv[1] : "/";

    file_info_t files[32];
    int n = sys_listdir(path, files, 32);
    if (n < 0) {
        printf("ls: %s: error %d\n", path, n);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        printf("  %s\n", files[i].name);
    }
    return 0;
}
