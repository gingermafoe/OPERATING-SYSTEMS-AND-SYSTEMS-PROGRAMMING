/**
 * @file zombie_demo.c
 * @brief Controlled demonstration of zombie process lifecycle and resolution.
 *
 * Concepts demonstrated:
 *   1. fork() child process
 *   2. Child terminates immediately via exit()
 *   3. Parent intentionally delays wait()/waitpid()
 *   4. Child's PCB remains in Linux kernel process table in state 'Z' (Zombie)
 *   5. Parent invokes waitpid() to read exit status and release PCB
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

static volatile sig_atomic_t g_timed_out = 0;

static void alarm_handler(int sig) {
    (void)sig;
    g_timed_out = 1;
}

int main(int argc, char *argv[]) {
    int timeout_sec = 60;
    if (argc > 1) {
        timeout_sec = atoi(argv[1]);
        if (timeout_sec <= 0) timeout_sec = 60;
    }

    printf("======================================================================\n");
    printf("  PROCESSSCOPE :: CONTROLLED ZOMBIE PROCESS DEMONSTRATION             \n");
    printf("======================================================================\n\n");

    pid_t parent_pid = getpid();
    printf("[Init] Parent Process running with PID: %d\n", parent_pid);
    printf("[Init] Forking child process...\n\n");

    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (child_pid == 0) {
        /* Child Process */
        printf("  [Child  PID %d] Created by Parent %d.\n", getpid(), getppid());
        printf("  [Child  PID %d] Executing termination via exit(42)...\n", getpid());
        exit(42);
    }

    /* Parent Process */
    /* Sleep 100ms to guarantee child terminates first */
    usleep(100000);

    printf("[Parent PID %d] Child process (PID %d) has called exit().\n", parent_pid, child_pid);
    printf("[Parent PID %d] Child is now in STATE 'Z' (Zombie / Defunct) because\n", parent_pid);
    printf("                the parent has NOT yet called wait() or waitpid().\n\n");
    printf(">>> OBSERVATION STEP <<<\n");
    printf("  Open another terminal and run either:\n");
    printf("    ./bin/processcope --zombies-only\n");
    printf("    ./bin/processcope --pid %d\n", child_pid);
    printf("  or inspect the ProcessScope web dashboard.\n\n");
    printf("  (Auto-safety timeout set to %d seconds)\n\n", timeout_sec);

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    alarm(timeout_sec);

    printf("Press [ENTER] to reap the zombie child with waitpid(), or wait for timeout: ");
    fflush(stdout);

    char buf[128];
    if (fgets(buf, sizeof(buf), stdin) == NULL && g_timed_out) {
        printf("\n[Parent PID %d] Safety timeout reached! Reaping zombie automatically...\n", parent_pid);
    } else {
        alarm(0); /* Cancel safety timer */
    }

    printf("\n[Parent PID %d] Calling waitpid(%d, &status, 0)...\n", parent_pid, child_pid);
    int status = 0;
    pid_t reaped_pid = waitpid(child_pid, &status, 0);

    if (reaped_pid < 0) {
        perror("waitpid failed");
        return 1;
    }

    printf("[Parent PID %d] Successfully reaped child PID %d!\n", parent_pid, reaped_pid);
    if (WIFEXITED(status)) {
        printf("[Parent PID %d] Child exit code acknowledged: %d\n", parent_pid, WEXITSTATUS(status));
    }

    printf("\n[Cleanup] The PCB entry has been freed from the Linux kernel process table.\n");
    printf("[Verify] Run ./bin/processcope --zombies-only again to verify the zombie is gone.\n");
    printf("======================================================================\n");

    return 0;
}
