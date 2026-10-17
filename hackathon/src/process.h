/**
 * @file process.h
 * @brief Core data structures and definitions for ProcessScope.
 *
 * ProcessScope: Zombie and Orphan Process Detection System
 * Course: Operating Systems and Systems Programming
 */

#ifndef PROCESSSCOPE_PROCESS_H
#define PROCESSSCOPE_PROCESS_H

#include <sys/types.h>
#include <time.h>
#include <stddef.h>
#include <stdbool.h>

#define PROCESS_COMM_MAX 256
#define INITIAL_PROCESS_CAPACITY 512

/**
 * @brief High-level classification of process role for OS analysis.
 */
typedef enum {
    CLASS_STANDARD = 0,         /**< Normal process with active parent */
    CLASS_ZOMBIE,               /**< Terminated process awaiting parent wait() (State 'Z') */
    CLASS_ADOPTED_CANDIDATE     /**< Process with PPID 1 (or candidate subreaper adoption) */
} ProcessClassification;

/**
 * @brief Detailed metadata extracted per accessible Linux process.
 */
typedef struct {
    pid_t pid;                                  /**< Process ID */
    pid_t ppid;                                 /**< Parent Process ID */
    char name[PROCESS_COMM_MAX];                /**< Executable / command name */
    char state;                                 /**< Single-char Linux state (Z, S, R, etc.) */
    const char *state_desc;                     /**< Human-readable state explanation */
    ProcessClassification classification;       /**< Heuristic / state classification */
    const char *classification_str;             /**< String: "zombie", "adopted_candidate", "standard" */
} ProcessInfo;

/**
 * @brief Snapshot of the system process table at a specific point in time.
 */
typedef struct {
    ProcessInfo *processes;                     /**< Dynamic array of scanned processes */
    size_t count;                               /**< Number of valid processes stored */
    size_t capacity;                            /**< Allocated array capacity */
    size_t total_scanned;                       /**< Total /proc entries inspected */
    size_t zombie_count;                        /**< Count of processes in 'Z' state */
    size_t adopted_candidate_count;             /**< Count of candidates adopted by PID 1 */
    size_t other_count;                         /**< Count of standard running/sleeping processes */
    time_t timestamp;                           /**< Epoch time when scan completed */
} ProcessSnapshot;

/**
 * @brief Filter options for terminal display or targeted queries.
 */
typedef struct {
    bool show_zombies_only;                     /**< Filter for zombies only */
    bool show_orphans_only;                     /**< Filter for adopted candidates only */
    bool json_output;                           /**< Emit machine-readable JSON */
    pid_t filter_pid;                           /**< Match specific PID (0 = any) */
    const char *filter_name;                    /**< Substring match on name (NULL = any) */
} ScanFilter;

#endif /* PROCESSSCOPE_PROCESS_H */
