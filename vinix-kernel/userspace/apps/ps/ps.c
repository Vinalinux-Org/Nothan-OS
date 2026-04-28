/* ps — list tasks by reading /proc */

#include "stdio.h"
#include "syscalls.h"
#include "user_syscall.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    process_info_t tasks[16];
    int n = sys_get_tasks(tasks, 16);
    if (n < 0) {
        printf("ps: error %d\n", n);
        return 1;
    }

    printf("\n%-6s%-24s%-10s\n", "PID", "NAME", "STATE");
    printf("%-6s%-24s%-10s\n", "---", "----", "-----");
    for (int i = 0; i < n; i++) {
        const char *st;
        switch (tasks[i].state) {
        case 0: st = "READY";   break;
        case 1: st = "RUNNING"; break;
        case 2: st = "BLOCKED"; break;
        case 3: st = "ZOMBIE";  break;
        default: st = "?";      break;
        }
        printf("%-6d%-24s%-10s\n", tasks[i].id, tasks[i].name, st);
    }
    printf("\n");
    return 0;
}
