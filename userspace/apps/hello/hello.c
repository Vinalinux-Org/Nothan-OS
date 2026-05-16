/* ============================================================
 * hello — NothCC compiler integration test.
 * Built by NothCC (Subset C -> ARMv7 ELF32), not arm-none-eabi-gcc.
 * ============================================================ */

#include "reflibc.h"

int add(int a, int b)
{
    return a + b;
}

int mul(int a, int b)
{
    int result = 0;
    int i = 0;
    while (i < b) {
        result = result + a;
        i = i + 1;
    }
    return result;
}

int main()
{
    print_str("Hello from NothCC!\n");
    print_str("Compiler: NothanOS NothCC (Subset C -> ARMv7)\n");
    print_str("\n");

    print_str("add(3, 4) = ");
    print_int(add(3, 4));
    print_str("\n");

    print_str("mul(6, 7) = ");
    print_int(mul(6, 7));
    print_str("\n");

    print_str("\ndone.\n");
    return 0;
}
