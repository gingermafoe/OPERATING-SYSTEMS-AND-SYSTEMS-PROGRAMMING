/**
 * @file main.c
 * @brief Command-line interface for ProcessScope.
 *
 * ProcessScope: Zombie and Orphan Process Detection System
 * Course: Operating Systems and Systems Programming
 */

#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROCESSSCOPE_VERSION "1.0.0"

static void print_usage(const char *prog_name) {
    printf("ProcessScope v%s — Zombie & Orphan Process Detection System\n", PROCESSSCOPE_VERSION);
    printf("Course: Operating Systems and Systems Programming\n\n");
    printf("Usage:\n");
    printf("  %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  --json              Emit machine-readable JSON to stdout\n");
    printf("  --zombies-only      Filter output to display only Zombie ('Z') processes\n");
    printf("  --orphans-only      Filter output to display only Candidate Adopted processes\n");
    printf("  --pid <PID>         Inspect a specific process ID\n");
    printf("  --name <NAME>       Filter by process name substring\n");
    printf("  -h, --help          Show this detailed help information\n");
    printf("  -v, --version       Display version and build info\n\n");
    printf("OS Concepts & Detection Methodology:\n");
    printf("  1. Zombie Process ('Z' / Defunct):\n");
    printf("     A process that has completed execution via exit(), but whose parent has not\n");
    printf("     yet read its exit status via wait()/waitpid(). Its PCB entry remains in the\n");
    printf("     process table to store the termination status code.\n\n");
    printf("  2. Adopted Candidate Process (PPID 1 / Subreaper):\n");
    printf("     In Linux, when a parent terminates before its child, the child becomes an orphan\n");
    printf("     and is reparented to init (PID 1) or an ancestor subreaper (PR_SET_CHILD_SUBREAPER).\n");
    printf("     Because a single snapshot cannot prove original parentage, snapshot entries\n");
    printf("     parented by PID 1 are labeled 'adopted_candidate'.\n");
}

int main(int argc, char *argv[]) {
    ScanFilter filter;
    memset(&filter, 0, sizeof(filter));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            filter.json_output = true;
        } else if (strcmp(argv[i], "--zombies-only") == 0) {
            filter.show_zombies_only = true;
        } else if (strcmp(argv[i], "--orphans-only") == 0) {
            filter.show_orphans_only = true;
        } else if (strcmp(argv[i], "--pid") == 0) {
            if (i + 1 < argc) {
                filter.filter_pid = (pid_t)atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: --pid requires an integer argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) {
                filter.filter_name = argv[++i];
            } else {
                fprintf(stderr, "Error: --name requires a string argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("ProcessScope version %s (Linux /proc analyzer)\n", PROCESSSCOPE_VERSION);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s. Use --help for usage.\n", argv[i]);
            return 1;
        }
    }

    ProcessSnapshot snapshot;
    if (scan_processes(&snapshot) != 0) {
        if (filter.json_output) {
            fprintf(stdout, "{\"error\": \"Failed to read /proc filesystem\"}\n");
        } else {
            fprintf(stderr, "Error: Unable to scan /proc. Ensure the /proc filesystem is mounted.\n");
        }
        return 1;
    }

    if (filter.json_output) {
        print_json(&snapshot, stdout);
    } else {
        print_terminal_table(&snapshot, &filter);
    }

    free_snapshot(&snapshot);
    return 0;
}
