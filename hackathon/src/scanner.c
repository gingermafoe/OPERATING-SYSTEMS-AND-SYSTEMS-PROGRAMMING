/**
 * @file scanner.c
 * @brief Implementation of /proc traversal, stat parser, and formatting routines.
 */

#define _POSIX_C_SOURCE 200809L

#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

/* ANSI Terminal Colors */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_DIM     "\033[2m"

int parse_stat_line(const char *line, pid_t *pid, char *name, size_t max_name_len, char *state, pid_t *ppid) {
    if (!line || !pid || !name || !state || !ppid || max_name_len == 0) {
        return -1;
    }

    /* Linux /proc/[pid]/stat format:
     *   pid (comm) state ppid ...
     * The comm field starts at the first '(' and terminates at the LAST ')'.
     */
    const char *open_paren = strchr(line, '(');
    const char *close_paren = strrchr(line, ')');

    if (!open_paren || !close_paren || close_paren <= open_paren) {
        return -1;
    }

    /* Parse PID preceding the opening parenthesis */
    char *endptr = NULL;
    long parsed_pid = strtol(line, &endptr, 10);
    if (endptr == line || parsed_pid <= 0) {
        return -1;
    }
    *pid = (pid_t)parsed_pid;

    /* Extract process name (comm) inside parentheses */
    size_t comm_len = (size_t)(close_paren - (open_paren + 1));
    if (comm_len >= max_name_len) {
        comm_len = max_name_len - 1;
    }
    memcpy(name, open_paren + 1, comm_len);
    name[comm_len] = '\0';

    /* Parse state and PPID following the closing parenthesis */
    const char *after_comm = close_paren + 1;
    char parsed_state = '\0';
    int parsed_ppid = 0;

    if (sscanf(after_comm, " %c %d", &parsed_state, &parsed_ppid) != 2) {
        return -1;
    }

    *state = parsed_state;
    *ppid = (pid_t)parsed_ppid;
    return 0;
}

const char *get_state_description(char state) {
    switch (state) {
        case 'R': return "Running (R)";
        case 'S': return "Sleeping / Interruptible (S)";
        case 'D': return "Disk Sleep / Uninterruptible (D)";
        case 'Z': return "Zombie / Defunct (Z)";
        case 'T': return "Stopped by job control (T)";
        case 't': return "Tracing stop (t)";
        case 'X': return "Dead (X)";
        case 'x': return "Dead (x)";
        case 'K': return "Wakekill (K)";
        case 'W': return "Waking (W)";
        case 'P': return "Parked (P)";
        case 'I': return "Idle Kernel Thread (I)";
        default:  return "Unknown";
    }
}

ProcessClassification classify_process(pid_t pid, pid_t ppid, char state) {
    if (state == 'Z') {
        return CLASS_ZOMBIE;
    }
    /*
     * Important OS Distinction:
     * In Linux, a snapshot cannot conclusively prove whether a process parented
     * by PID 1 was initially spawned as an orphan and reparented, or was spawned
     * directly by init/systemd. We classify these as "adopted_candidate".
     */
    if (ppid == 1 && pid != 1) {
        return CLASS_ADOPTED_CANDIDATE;
    }
    return CLASS_STANDARD;
}

const char *classification_to_string(ProcessClassification cls) {
    switch (cls) {
        case CLASS_ZOMBIE:            return "zombie";
        case CLASS_ADOPTED_CANDIDATE: return "adopted_candidate";
        case CLASS_STANDARD:
        default:                      return "standard";
    }
}

static int compare_process_pid(const void *a, const void *b) {
    const ProcessInfo *p1 = (const ProcessInfo *)a;
    const ProcessInfo *p2 = (const ProcessInfo *)b;
    if (p1->pid < p2->pid) return -1;
    if (p1->pid > p2->pid) return 1;
    return 0;
}

int scan_processes(ProcessSnapshot *snapshot) {
    if (!snapshot) {
        return -1;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->capacity = INITIAL_PROCESS_CAPACITY;
    snapshot->processes = (ProcessInfo *)malloc(snapshot->capacity * sizeof(ProcessInfo));
    if (!snapshot->processes) {
        return -1;
    }

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        free(snapshot->processes);
        snapshot->processes = NULL;
        return -1;
    }

    struct dirent *entry;
    char path_buf[512];
    char line_buf[2048];

    while ((entry = readdir(proc_dir)) != NULL) {
        /* Numeric directories correspond to active PIDs */
        if (!isdigit((unsigned char)entry->d_name[0])) {
            continue;
        }

        snapshot->total_scanned++;

        int written = snprintf(path_buf, sizeof(path_buf), "/proc/%s/stat", entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(path_buf)) {
            continue;
        }

        FILE *fp = fopen(path_buf, "r");
        if (!fp) {
            /* Process likely exited between readdir and fopen (normal on Linux) */
            continue;
        }

        if (!fgets(line_buf, sizeof(line_buf), fp)) {
            fclose(fp);
            continue;
        }
        fclose(fp);

        pid_t pid = 0;
        pid_t ppid = 0;
        char state = '\0';
        char comm[PROCESS_COMM_MAX];

        if (parse_stat_line(line_buf, &pid, comm, sizeof(comm), &state, &ppid) != 0) {
            continue;
        }

        /* Resize dynamic buffer if needed */
        if (snapshot->count >= snapshot->capacity) {
            size_t new_cap = snapshot->capacity * 2;
            ProcessInfo *resized = (ProcessInfo *)realloc(snapshot->processes, new_cap * sizeof(ProcessInfo));
            if (!resized) {
                break; /* Keep what we have scanned so far */
            }
            snapshot->processes = resized;
            snapshot->capacity = new_cap;
        }

        ProcessInfo *proc = &snapshot->processes[snapshot->count++];
        proc->pid = pid;
        proc->ppid = ppid;
        strncpy(proc->name, comm, sizeof(proc->name) - 1);
        proc->name[sizeof(proc->name) - 1] = '\0';
        proc->state = state;
        proc->state_desc = get_state_description(state);
        proc->classification = classify_process(pid, ppid, state);
        proc->classification_str = classification_to_string(proc->classification);

        if (proc->classification == CLASS_ZOMBIE) {
            snapshot->zombie_count++;
        } else if (proc->classification == CLASS_ADOPTED_CANDIDATE) {
            snapshot->adopted_candidate_count++;
        } else {
            snapshot->other_count++;
        }
    }

    closedir(proc_dir);
    snapshot->timestamp = time(NULL);

    /* Sort entries deterministically by PID */
    if (snapshot->count > 1) {
        qsort(snapshot->processes, snapshot->count, sizeof(ProcessInfo), compare_process_pid);
    }

    return 0;
}

void free_snapshot(ProcessSnapshot *snapshot) {
    if (snapshot) {
        if (snapshot->processes) {
            free(snapshot->processes);
            snapshot->processes = NULL;
        }
        snapshot->count = 0;
        snapshot->capacity = 0;
    }
}

char *json_escape_string(const char *input) {
    if (!input) {
        return strdup("");
    }

    size_t in_len = strlen(input);
    /* Worst case: every char escaped to \u00XX (6 bytes) */
    char *out = (char *)malloc(in_len * 6 + 1);
    if (!out) {
        return NULL;
    }

    char *dst = out;
    for (size_t i = 0; i < in_len; i++) {
        unsigned char c = (unsigned char)input[i];
        switch (c) {
            case '\"': *dst++ = '\\'; *dst++ = '\"'; break;
            case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
            case '\b': *dst++ = '\\'; *dst++ = 'b'; break;
            case '\f': *dst++ = '\\'; *dst++ = 'f'; break;
            case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
            case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
            case '\t': *dst++ = '\\'; *dst++ = 't'; break;
            default:
                if (c < 32 || c == 127) {
                    dst += sprintf(dst, "\\u%04x", c);
                } else {
                    *dst++ = (char)c;
                }
                break;
        }
    }
    *dst = '\0';
    return out;
}

void print_terminal_table(const ProcessSnapshot *snapshot, const ScanFilter *filter) {
    if (!snapshot) {
        return;
    }

    int is_tty = isatty(fileno(stdout));

    printf("\n");
    if (is_tty) {
        printf("%s%s========================================================================================%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
        printf("%s%s  PROCESSSCOPE :: LINUX PROCESS TREE & RELATIONSHIP SCANNER                              %s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
        printf("%s%s========================================================================================%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    } else {
        printf("========================================================================================\n");
        printf("  PROCESSSCOPE :: LINUX PROCESS TREE & RELATIONSHIP SCANNER                              \n");
        printf("========================================================================================\n");
    }

    printf("  %-8s %-8s %-6s %-20s %-30s\n", "PID", "PPID", "STATE", "CLASSIFICATION", "COMMAND / NAME");
    printf("  --------------------------------------------------------------------------------------\n");

    size_t displayed_count = 0;

    for (size_t i = 0; i < snapshot->count; i++) {
        const ProcessInfo *p = &snapshot->processes[i];

        /* Filter checks */
        if (filter) {
            if (filter->show_zombies_only && p->classification != CLASS_ZOMBIE) {
                continue;
            }
            if (filter->show_orphans_only && p->classification != CLASS_ADOPTED_CANDIDATE) {
                continue;
            }
            if (filter->filter_pid > 0 && p->pid != filter->filter_pid) {
                continue;
            }
            if (filter->filter_name && !strstr(p->name, filter->filter_name)) {
                continue;
            }
        }

        displayed_count++;

        if (is_tty) {
            const char *state_color = COLOR_RESET;
            const char *badge = "";

            if (p->classification == CLASS_ZOMBIE) {
                state_color = COLOR_RED;
                badge = "[ZOMBIE]";
            } else if (p->classification == CLASS_ADOPTED_CANDIDATE) {
                state_color = COLOR_YELLOW;
                badge = "[ADOPTED CANDIDATE]";
            } else {
                state_color = (p->state == 'R') ? COLOR_GREEN : COLOR_DIM;
                badge = "[STANDARD]";
            }

            printf("  %-8d %-8d %s%-6c%s %s%-20s%s %-30s\n",
                   p->pid, p->ppid,
                   state_color, p->state, COLOR_RESET,
                   state_color, badge, COLOR_RESET,
                   p->name);
        } else {
            const char *badge = "STANDARD";
            if (p->classification == CLASS_ZOMBIE) badge = "ZOMBIE";
            else if (p->classification == CLASS_ADOPTED_CANDIDATE) badge = "ADOPTED_CANDIDATE";

            printf("  %-8d %-8d %-6c %-20s %-30s\n",
                   p->pid, p->ppid, p->state, badge, p->name);
        }
    }

    printf("  --------------------------------------------------------------------------------------\n");
    printf("  Summary: Total Active: %zu | Zombies (Z): %zu | Adopted Candidates: %zu | Displayed: %zu\n",
           snapshot->count, snapshot->zombie_count, snapshot->adopted_candidate_count, displayed_count);

    if (snapshot->zombie_count > 0) {
        if (is_tty) {
            printf("  %s%s[!] ALERT: %zu zombie process(es) detected in defunct state awaiting parent wait().%s\n",
                   COLOR_BOLD, COLOR_RED, snapshot->zombie_count, COLOR_RESET);
        } else {
            printf("  [!] ALERT: %zu zombie process(es) detected in defunct state awaiting parent wait().\n",
                   snapshot->zombie_count);
        }
    }
    printf("\n");
}

void print_json(const ProcessSnapshot *snapshot, FILE *out) {
    if (!snapshot || !out) {
        return;
    }

    fprintf(out, "{\n");
    fprintf(out, "  \"timestamp\": %ld,\n", (long)snapshot->timestamp);
    fprintf(out, "  \"total_scanned\": %zu,\n", snapshot->total_scanned);
    fprintf(out, "  \"counts\": {\n");
    fprintf(out, "    \"total\": %zu,\n", snapshot->count);
    fprintf(out, "    \"zombies\": %zu,\n", snapshot->zombie_count);
    fprintf(out, "    \"adopted_candidates\": %zu,\n", snapshot->adopted_candidate_count);
    fprintf(out, "    \"standard\": %zu\n", snapshot->other_count);
    fprintf(out, "  },\n");
    fprintf(out, "  \"meta\": {\n");
    fprintf(out, "    \"note\": \"Snapshot PPID 1 classification is labeled adopted_candidate because a single snapshot cannot confirm prior parentage; subreapers may also adopt.\"\n");
    fprintf(out, "  },\n");
    fprintf(out, "  \"processes\": [\n");

    for (size_t i = 0; i < snapshot->count; i++) {
        const ProcessInfo *p = &snapshot->processes[i];
        char *esc_name = json_escape_string(p->name);
        char *esc_desc = json_escape_string(p->state_desc);

        fprintf(out, "    {\n");
        fprintf(out, "      \"pid\": %d,\n", (int)p->pid);
        fprintf(out, "      \"ppid\": %d,\n", (int)p->ppid);
        fprintf(out, "      \"name\": \"%s\",\n", esc_name ? esc_name : "");
        fprintf(out, "      \"state\": \"%c\",\n", p->state);
        fprintf(out, "      \"state_desc\": \"%s\",\n", esc_desc ? esc_desc : "");
        fprintf(out, "      \"classification\": \"%s\"\n", p->classification_str);
        fprintf(out, "    }%s\n", (i + 1 < snapshot->count) ? "," : "");

        free(esc_name);
        free(esc_desc);
    }

    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
}
