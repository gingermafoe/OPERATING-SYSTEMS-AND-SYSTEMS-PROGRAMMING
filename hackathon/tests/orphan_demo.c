/**
 * @file orphan_demo.c
 * @brief Controlled demonstration of orphan process creation and reparenting.
 *
 * Concepts demonstrated:
 *   1. fork() child process
 *   2. Parent records initial relationship and terminates immediately
 *   3. Child continues running in the background
 *   4. Linux kernel reparents the orphaned child to init (PID 1) or an ancestor subreaper
 *   5. Child verifies PPID change via getppid()
 *   6. Child terminates after bounded duration to prevent lingering background tasks
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    int duration_sec = 10;
    if (argc > 1) {
        duration_sec = atoi(argv[1]);
        if (duration_sec <= 0 || duration_sec > 60) duration_sec = 10;
    }

    printf("======================================================================\n");
    printf("  PROCESSSCOPE :: CONTROLLED ORPHAN REPARENTING DEMONSTRATION         \n");
    printf("======================================================================\n\n");

    pid_t orig_parent = getpid();
    printf("[Init] Original Parent Process PID: %d\n", orig_parent);
    printf("[Init] Forking child process...\n\n");

    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (child_pid > 0) {
        /* Parent process exits immediately */
        printf("[Parent PID %d] Child process created with PID %d.\n", orig_parent, child_pid);
        printf("[Parent PID %d] Parent is exiting now to orphan the child...\n", orig_parent);
        printf("[Parent PID %d] Terminated.\n\n", orig_parent);
        exit(0);
    }

    /* Child Process execution continues */
    /* Sleep 200ms to allow parent process to fully exit and trigger kernel reparenting */
    usleep(200000);

    pid_t new_ppid = getppid();

    printf("\n  [Child Process PID %d] =============================================\n", getpid());
    printf("  [Child Process PID %d] Original Parent PID was: %d\n", getpid(), orig_parent);
    printf("  [Child Process PID %d] New Adoptive Parent PID is: %d\n", getpid(), new_ppid);

    if (new_ppid == 1) {
        printf("  [Reparenting Confirmed] Child was adopted directly by init / systemd (PID 1)!\n");
    } else {
        printf("  [Reparenting Confirmed] Child was adopted by Subreaper PID %d\n", new_ppid);
        printf("                          (Configured via PR_SET_CHILD_SUBREAPER, e.g., systemd --user)\n");
    }

    printf("  >>> OBSERVATION WINDOW (%d SECONDS) <<<\n", duration_sec);
    printf("  Inspect this process in ProcessScope using:\n");
    printf("    ./bin/processcope --pid %d\n", getpid());
    printf("    ./bin/processcope --orphans-only\n");
    printf("  or on the ProcessScope Web Dashboard.\n\n");

    for (int i = duration_sec; i > 0; i -= 2) {
        printf("  [Child PID %d] Still running (PPID %d). Auto-terminating in %d seconds...\n",
               getpid(), getppid(), i);
        sleep(2);
    }

    printf("  [Child PID %d] Bounded demo duration complete. Exiting cleanly now.\n", getpid());
    printf("  ====================================================================\n\n");

    return 0;
}
