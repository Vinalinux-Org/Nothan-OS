/* echo — write arguments to stdout separated by spaces */

#include "stdio.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) printf(" ");
        printf("%s", argv[i]);
    }
    printf("\n");
    return 0;
}
