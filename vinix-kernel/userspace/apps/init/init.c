/* ============================================================
 * init — PID 1. Forks the shell, waits for it to exit, respawns.
 *
 * Boot flow:
 *   kernel payload (init.bin) → _start → main → fork/exec "sh"
 *   shell on FAT32 as /sh; first-boot requires copying shell.elf
 *   to the card as plain "sh" (8.3 no extension).
 *
 * If exec fails (no shell on card), init pauses a beat and retries
 * rather than tight-looping the fork+exec path.
 * ============================================================ */

#include "stdio.h"
#include "unistd.h"
#include "sys/wait.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("\n[INIT] VinixOS init (pid=%d)\n", getpid());
    printf("[INIT] launching /sh...\n");

    while (1) {
        int pid = fork();
        if (pid < 0) {
            printf("[INIT] fork failed, pausing\n");
            sleep(1);
            continue;
        }

        if (pid == 0) {
            /* Child replaces itself with the shell. Shell lives at
             * /bin/sh per the rootfs layout built by the deploy
             * script; fall back to a plain "sh" at the root for
             * cards produced before the FHS reorganisation. */
            char *argv[] = { "sh", 0 };
            execve("/bin/sh", argv, 0);
            execve("sh", argv, 0);
            printf("[INIT] exec sh failed — /bin/sh missing\n");
            _exit(127);
        }

        int status = 0;
        int reaped = waitpid(-1, &status, 0);
        printf("[INIT] shell pid=%d exited (status=%d), respawning\n",
               reaped, status);
        sleep(1);
    }
    return 0;
}
