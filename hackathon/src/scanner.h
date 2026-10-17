/**
 * @file scanner.h
 * @brief Scanner interface for reading /proc entries and parsing process data.
 */

#ifndef PROCESSSCOPE_SCANNER_H
#define PROCESSSCOPE_SCANNER_H

#include "process.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Robust parser for a single line from /proc/[pid]/stat.
 *
 * Accounts for arbitrary process names containing spaces and nested parentheses.
 * Per the Linux kernel specification, the comm field is enclosed by '(' and
 * ends at the LAST occurrence of ')' in the line.
 *
 * @param line Input line from /proc/[pid]/stat.
 * @param pid Output extracted PID.
 * @param name Output extracted process name buffer.
 * @param max_name_len Capacity of name buffer.
 * @param state Output process state character.
 * @param ppid Output extracted PPID.
 * @return 0 on success, -1 on parse failure.
 */
int parse_stat_line(const char *line, pid_t *pid, char *name, size_t max_name_len, char *state, pid_t *ppid);

/**
 * @brief Returns a friendly description for a Linux process state character.
 */
const char *get_state_description(char state);

/**
 * @brief Classifies a process based on state and parent PID.
 */
ProcessClassification classify_process(pid_t pid, pid_t ppid, char state);

/**
 * @brief Returns string representation of classification enum.
 */
const char *classification_to_string(ProcessClassification cls);

/**
 * @brief Scans /proc for all numeric process entries, reading /proc/[pid]/stat safely.
 *
 * Handles permission errors (EACCES) and disappearing processes (ENOENT)
 * gracefully without crashing.
 *
 * @param snapshot Pointer to ProcessSnapshot structure to populate.
 * @return 0 on success, -1 on fatal error.
 */
int scan_processes(ProcessSnapshot *snapshot);

/**
 * @brief Releases memory dynamically allocated for snapshot.
 */
void free_snapshot(ProcessSnapshot *snapshot);

/**
 * @brief Formats and prints a terminal table with color-coded alerts and summary metrics.
 */
void print_terminal_table(const ProcessSnapshot *snapshot, const ScanFilter *filter);

/**
 * @brief Emits a strictly compliant JSON document containing the process snapshot.
 */
void print_json(const ProcessSnapshot *snapshot, FILE *out);

/**
 * @brief Safely escapes JSON string characters (quotes, backslashes, controls).
 * The caller must free() the returned pointer.
 */
char *json_escape_string(const char *input);

#ifdef __cplusplus
}
#endif

#endif /* PROCESSSCOPE_SCANNER_H */
