/* ============================================================
 * uname — first external VinixOS program using vinixlibc.
 * ============================================================ */

#include "stdio.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("VinixOS 0.1 armv7-a beaglebone-black\n");
    return 0;
}
