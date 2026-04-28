/* ============================================================
 * hello — proves vinixlibc works end-to-end: malloc, fork-safe
 * printf, strlen, all from external ELF loaded via sys_exec.
 * ============================================================ */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("Hello from an external VinixOS program!\n");
    printf("  pid=%d  ppid=%d\n", getpid(), getppid());

    /* Exercise the K&R-style allocator. */
    char *msg = (char *)malloc(64);
    if (msg) {
        strcpy(msg, "  malloc works: ");
        printf("%s%d bytes returned, len=%d\n", msg, 64, (int)strlen(msg));
        free(msg);
    }

    return 0;
}
