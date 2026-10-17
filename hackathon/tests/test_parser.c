/**
 * @file test_parser.c
 * @brief Unit tests for /proc/[pid]/stat line parsing and JSON string escaping.
 *
 * Verifies edge cases including spaces and nested parentheses in process names.
 */

#include "../src/scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, desc) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", desc); \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", desc, __LINE__); \
    } \
} while (0)

int main(void) {
    printf("======================================================================\n");
    printf("  PROCESSSCOPE :: UNIT TESTS (STAT PARSER & JSON ESCAPING)            \n");
    printf("======================================================================\n\n");

    pid_t pid = 0;
    pid_t ppid = 0;
    char state = '\0';
    char name[PROCESS_COMM_MAX];

    /* Test 1: Standard single-word command */
    {
        const char *line = "1234 (systemd) S 0 1234 1234 0 -1 4194560 123 456 0 0";
        int res = parse_stat_line(line, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res == 0, "Parse standard stat line");
        TEST_ASSERT(pid == 1234, "Verify PID 1234");
        TEST_ASSERT(strcmp(name, "systemd") == 0, "Verify name 'systemd'");
        TEST_ASSERT(state == 'S', "Verify state 'S'");
        TEST_ASSERT(ppid == 0, "Verify PPID 0");
    }

    /* Test 2: Process name with spaces */
    {
        const char *line = "5678 (Web Content Worker) R 1234 5678 5678 0 -1 4194304";
        int res = parse_stat_line(line, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res == 0, "Parse process name with spaces");
        TEST_ASSERT(pid == 5678, "Verify PID 5678");
        TEST_ASSERT(strcmp(name, "Web Content Worker") == 0, "Verify name 'Web Content Worker'");
        TEST_ASSERT(state == 'R', "Verify state 'R'");
        TEST_ASSERT(ppid == 1234, "Verify PPID 1234");
    }

    /* Test 3: Process name with internal parentheses */
    {
        const char *line = "9012 (worker (gpu) thread) S 5678 9012 9012 0 -1 0";
        int res = parse_stat_line(line, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res == 0, "Parse process name with nested parentheses");
        TEST_ASSERT(pid == 9012, "Verify PID 9012");
        TEST_ASSERT(strcmp(name, "worker (gpu) thread") == 0, "Verify name 'worker (gpu) thread'");
        TEST_ASSERT(state == 'S', "Verify state 'S'");
        TEST_ASSERT(ppid == 5678, "Verify PPID 5678");
    }

    /* Test 4: Zombie process state 'Z' */
    {
        const char *line = "4444 (defunct_proc) Z 1000 4444 1000 0 -1 0";
        int res = parse_stat_line(line, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res == 0, "Parse zombie process line");
        TEST_ASSERT(state == 'Z', "Verify state 'Z'");
        TEST_ASSERT(classify_process(pid, ppid, state) == CLASS_ZOMBIE, "Classify as CLASS_ZOMBIE");
    }

    /* Test 5: Candidate Adopted process (PPID == 1, not PID 1 itself) */
    {
        pid = 2000;
        ppid = 1;
        state = 'S';
        TEST_ASSERT(classify_process(pid, ppid, state) == CLASS_ADOPTED_CANDIDATE, "Classify PPID 1 as CLASS_ADOPTED_CANDIDATE");
    }

    /* Test 6: PID 1 itself should be CLASS_STANDARD, not adopted candidate */
    {
        pid = 1;
        ppid = 0;
        state = 'S';
        TEST_ASSERT(classify_process(pid, ppid, state) == CLASS_STANDARD, "PID 1 classified as CLASS_STANDARD");
    }

    /* Test 7: Malformed input lines rejected safely */
    {
        const char *no_parens = "1234 malformed line without parens";
        int res1 = parse_stat_line(no_parens, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res1 == -1, "Safely reject line without parentheses");

        const char *no_state = "1234 (proc_name)";
        int res2 = parse_stat_line(no_state, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res2 == -1, "Safely reject truncated stat line");

        int res3 = parse_stat_line(NULL, &pid, name, sizeof(name), &state, &ppid);
        TEST_ASSERT(res3 == -1, "Safely reject NULL pointer");
    }

    /* Test 8: JSON string escaping */
    {
        const char *tricky_name = "test\"process\\name\nwith\ttabs";
        char *escaped = json_escape_string(tricky_name);
        TEST_ASSERT(escaped != NULL, "Generate escaped JSON string");
        TEST_ASSERT(strstr(escaped, "\\\"") != NULL, "Verify quote escaped");
        TEST_ASSERT(strstr(escaped, "\\\\") != NULL, "Verify backslash escaped");
        TEST_ASSERT(strstr(escaped, "\\n") != NULL, "Verify newline escaped");
        TEST_ASSERT(strstr(escaped, "\\t") != NULL, "Verify tab escaped");
        free(escaped);
    }

    printf("\n----------------------------------------------------------------------\n");
    printf("  Results: %d of %d assertions passed.\n", g_tests_passed, g_tests_run);
    printf("======================================================================\n\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
