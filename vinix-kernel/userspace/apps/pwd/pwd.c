/* pwd — print working directory
 *
 * VinixOS has no chdir yet, so getcwd always returns "/". Kept
 * for interface compatibility with future utilities. */

#include "stdio.h"
#include "unistd.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[64];
    if (getcwd(buf, sizeof(buf)) == 0) {
        printf("pwd: getcwd failed\n");
        return 1;
    }
    printf("%s\n", buf);
    return 0;
}
