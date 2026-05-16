/* ============================================================
 * uname — first external NothanOS program using nothanlibc.
 * ============================================================ */

#include "stdio.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("NothanOS 0.1 armv7-a beaglebone-black\n");
    return 0;
}
