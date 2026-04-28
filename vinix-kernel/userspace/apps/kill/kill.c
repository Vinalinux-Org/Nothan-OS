/* kill — deliver SIGKILL by default */

#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "unistd.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return 1;
    }
    int pid = atoi(argv[1]);
    if (kill(pid, SIGKILL) < 0) {
        printf("kill: pid %d: cannot signal\n", pid);
        return 1;
    }
    return 0;
}
