# ProcessScope: Zombie and Orphan Process Detection System

**Course:** Operating Systems and Systems Programming  
**Category:** Systems Programming & Process Management  
**Target Environment:** Ubuntu Linux  
**Implementation Languages:** C (C11), Python 3 (Standard Library), HTML5, CSS3, JavaScript (ES6)

---

## 1. Abstract

In POSIX-compliant multi-tasking operating systems, process creation, execution, and termination follow structured kernel mechanisms. Anomalous lifecycle terminations commonly produce **zombie processes**—terminated processes whose parent has failed to read their exit status—and **orphan processes**—processes whose parent terminates prematurely, triggering automatic kernel reparenting.

This project introduces **ProcessScope**, a process inspection and monitoring architecture developed for Linux systems. ProcessScope provides a high-performance C11 engine that traverses the Linux virtual `/proc` filesystem, implements an unambiguous string parser capable of handling arbitrary process names containing spaces and nested parentheses, and classifies processes based on their execution state and parent-child hierarchy.

ProcessScope pairs this core scanner with a zero-external-dependency Python HTTP service and a web-based real-time dashboard. In addition, the system includes controlled laboratory demonstration fixtures that safely recreate the creation and dissolution of zombies and the reparenting of orphan processes.

---

## 2. Introduction

The process management subsystem of an operating system controls process scheduling, address space isolation, signal dispatching, and process cleanup. In Unix and Linux architectures, process creation is separated into two system calls: `fork()`, which creates an exact duplicate address space of the calling process, and `execve()`, which replaces the duplicate process image with a new executable.

When a process calls `exit()`, the kernel deallocates its virtual memory pages, file descriptor tables, and thread context. However, a remnant of the process must persist in kernel memory: the **Process Control Block (PCB)**, which stores the 32-bit exit status code and accounting statistics. The PCB is removed only when the parent process invokes `wait()` or `waitpid()`.

If the parent process fails to invoke `wait()`, the dead process enters the **Zombie** (defunct, state `Z`) state. While zombies consume zero CPU cycles and no user-space memory, they retain a unique Process Identification Number (PID). Because the system maintains a finite PID table (`/proc/sys/kernel/pid_max`), an unchecked proliferation of zombie processes can exhaust available PIDs, preventing the operating system from spawning new processes.

Conversely, if a parent process terminates before its child, the surviving child becomes an **Orphan**. The Linux kernel handles orphan processes through automatic reparenting, transferring the child's PPID to the root init system (`systemd` / PID 1) or an ancestor subreaper process.

ProcessScope was designed and implemented to provide systems engineers, students, and educators with an authoritative tool to inspect, analyze, and demonstrate these critical process lifecycle phenomena.

---

## 3. Problem Statement

Standard administrative tools such as `ps(1)`, `top(1)`, and `htop(1)` display lists of running processes but suffer from several pedagogical and operational limitations:

1. **Lack of Proactive Classification:** General process viewers treat zombie processes as mere text entries among hundreds of normal tasks, lacking dedicated alert mechanisms.
2. **Ambiguous Parsing Vulnerabilities:** Reading `/proc/[pid]/stat` using naive whitespace-based tokenizers (such as `scanf("%d %s %c %d")`) fails when executable names contain spaces or parentheses, leading to corrupted field offsets.
3. **Misleading Orphan Heuristics:** Many educational and prototype tools incorrectly assert that any process possessing `PPID == 1` is an orphan. In production Linux environments, system services and daemons are spawned directly by `systemd`. A single point-in-time snapshot cannot definitively establish the prior lineage of a process.
4. **Lack of Safe Educational Fixtures:** Demonstrating zombie generation and orphan reparenting in classroom or hackathon environments often involves unmanaged bash scripts or runaway background loops that leave lingering tasks on the host system.

---

## 4. Objectives

The primary objectives of the ProcessScope project are:

1. **Develop a Safe, High-Performance C Scanner:** Traverse the Linux `/proc` filesystem directly using POSIX standard interfaces (`opendir`, `readdir`, `fopen`, `fgets`) without invoking external shells or shell utilities.
2. **Implement Resilient Parsing:** Develop a robust parser for `/proc/[pid]/stat` that reliably parses process names containing spaces and multiple nested parentheses by finding the terminal closing parenthesis delimiter.
3. **Establish Clear Classification Semantics:**
   - Detect confirmed zombies by checking for process state `Z`.
   - Identify candidate adopted processes using `PPID == 1` (or subreaper) while explicitly documenting the snapshot heuristic boundary.
   - Maintain normal classification for active parent-child relationships.
4. **Build Controlled Demonstration Programs:**
   - Create `zombie_demo.c` to fork a child, simulate parent delay without `wait()`, allow user inspection, and reap the zombie on demand with `waitpid()`.
   - Create `orphan_demo.c` to fork a child, terminate the parent, observe kernel reparenting in real time, and cleanly terminate within a bounded timeframe.
5. **Construct a Professional Web Dashboard:** Build a responsive, dark-navy themed cybersecurity monitoring interface using pure HTML5, CSS3, and JavaScript that displays telemetry metrics, provides real-time search, filters by classification and state, and displays process metadata modals.
6. **Deploy a Zero-Dependency API Gateway:** Implement an authoritative local HTTP backend using only the Python 3 standard library that executes the C binary safely and validates JSON output without risk of command injection.

---

## 5. Existing System and Limitations

The table below contrasts conventional system tools with the proposed ProcessScope architecture:

| Feature / Capability | `ps aux` / `top` | Conventional Educational Scripts | ProcessScope |
| :--- | :--- | :--- | :--- |
| **Language / Foundation** | C (procps-ng) | Bash / Python wrappers | C11 + Python Stdlib + Vanilla Web |
| **Parsing Safety** | High (Internal kernel parser) | Poor (Often splits on space) | High (Rightmost parenthesis delimiter) |
| **Zombie Alerting** | None (Listed in text output) | Variable / Crude `grep` | Dedicated dashboard alert banner & metrics |
| **Orphan Classification** | None | Often falsely claims `PPID 1` = Orphan | Categorizes as `adopted_candidate` with documented heuristics |
| **Controlled Fixtures** | None | Ad-hoc background loops | Bounded, auto-timed C demonstration programs |
| **Web Interface** | None | Third-party dependencies (Node/Flask) | Native Vanilla JS/CSS + Python Stdlib |

---

## 6. Proposed System

ProcessScope addresses existing limitations by establishing a structured, modular pipeline:

1. **Kernel Procfs Traversal (`src/scanner.c`):** Direct numeric directory scan in `/proc` reading `/proc/[pid]/stat`.
2. **Rightmost Parenthesis Boundary Parser:** Correctly isolates the `comm` string from PIDs, states, and PPIDs.
3. **Structured Classification Engine:** Maps Linux state characters to human-readable descriptors and categorizes each task into `CLASS_ZOMBIE`, `CLASS_ADOPTED_CANDIDATE`, or `CLASS_STANDARD`.
4. **Dual Output Modalities:** Emits either an ANSI-colored terminal table with summary statistics or a machine-readable JSON document.
5. **Local Python API Server (`dashboard/server.py`):** Runs locally on `127.0.0.1`, safely invoking `./bin/processcope --json` via `subprocess.run` with timeouts and strict error catching.
6. **Real-Time Web Dashboard (`dashboard/static/`):** Automatically polls the API every 3 seconds, rendering interactive telemetry cards, search filters, state filters, and OS conceptual guides.

---

## 7. Architecture

The architecture of ProcessScope separates data acquisition, data validation, and presentation:

```
[ Linux Kernel /proc Filesystem ]
               |
               v (Read-only /proc/[pid]/stat)
[ ProcessScope C Scanner Engine (bin/processcope) ]
      |                                  |
      v (Terminal Display)               v (--json flag)
[ ANSI Terminal Table ]          [ JSON Data Stream ]
                                         |
                                         v
                         [ Python 3 HTTP Server (server.py) ]
                                         |
                                         v (REST JSON / HTTP)
                         [ Web Dashboard (index.html / app.js) ]
```

---

## 8. Algorithms and Detection Logic

### 8.1 Stat Line Delimiter Algorithm
The Linux kernel format for `/proc/[pid]/stat` is:
```
pid (command_name) state ppid pgrp session tty_nr tpgid flags ...
```
Because `command_name` can contain arbitrary spaces, tabs, and parentheses, naive token parsing fails. The algorithm implemented in `src/scanner.c` is:

```c
1. Find first occurrence of '(' using strchr(line, '(').
2. Parse preceding numeric characters as PID using strtol().
3. Find LAST occurrence of ')' using strrchr(line, ')').
4. Extract all characters between first '(' and last ')' as process name (comm).
5. Null-terminate process name buffer.
6. Parse subsequent characters after last ')' using sscanf(after_comm, " %c %d", &state, &ppid).
7. Validate that sscanf successfully parsed exactly 2 tokens.
```

### 8.2 Classification Decision Logic

```
IF process_state == 'Z' THEN
    classification = CLASS_ZOMBIE
ELSE IF ppid == 1 AND pid != 1 THEN
    classification = CLASS_ADOPTED_CANDIDATE
ELSE
    classification = CLASS_STANDARD
END IF
```

---

## 9. Implementation

### 9.1 Core Files & Directory Layout
- `src/process.h`: Core data types, including `ProcessInfo`, `ProcessSnapshot`, `ScanFilter`, and enumeration types.
- `src/scanner.h` & `src/scanner.c`: Direct `/proc` reader, parsing algorithms, JSON serializer, and terminal table formatter.
- `src/main.c`: Command-line interface handling `--json`, `--zombies-only`, `--orphans-only`, `--pid`, and `--name` options.
- `tests/zombie_demo.c`: Interactive zombie generation and `waitpid()` reaping demo.
- `tests/orphan_demo.c`: Controlled orphan reparenting verification demo.
- `tests/test_parser.c`: Strict C unit test suite checking stat parsing and JSON escaping.
- `tests/test_suite.py`: Automated end-to-end integration test runner.
- `dashboard/server.py`: Standard library Python HTTP service.
- `dashboard/static/`: Vanilla HTML5, CSS3, and JavaScript frontend.

### 9.2 Error Resilience Strategies
- **Disappearing Processes:** Processes terminating mid-scan return `ENOENT` on `fopen()`. The scanner handles this as a normal event and continues to the next entry.
- **Permission Denials:** Restricted `/proc` entries return `EACCES`. The scanner gracefully skips them without aborting.
- **Buffer Growth:** The snapshot array uses dynamic geometric reallocation (`malloc`/`realloc`), starting at 512 entries and doubling as necessary.
- **Safe JSON Escaping:** Characters such as `"`, `\`, newlines, and control bytes `< 32` are transformed into valid escape sequences or `\u00XX` unicode notation.

---

## 10. Testing Methodology

Testing was executed across four validation stages:

1. **Unit Testing (`bin/test_parser`):**
   - Verified parsing of standard command names (`systemd`).
   - Verified names containing spaces (`Web Content Worker`).
   - Verified names containing nested parentheses (`worker (gpu) thread`).
   - Verified error handling on malformed lines and NULL pointers.
   - Verified JSON escaping routines.
2. **Integration Testing (`tests/test_suite.py`):**
   - Verified CLI exit codes, table formatting, and summary calculations.
   - Validated JSON structure against Python `json.loads`.
   - Verified process field schema consistency.
3. **Controlled Lifecycle Testing:**
   - **Zombie Lifecycle:** Forked `zombie_demo`, verified appearance of state `Z` in scanner JSON output, sent reap signal via standard input, verified `waitpid()` resolution, and confirmed the PID was purged from `/proc`.
   - **Orphan Lifecycle:** Executed `orphan_demo`, allowed parent to exit, and confirmed child process PPID changed to 1 or subreaper PID.
4. **Browser & UI Testing:**
   - Verified API endpoints (`/api/health`, `/api/processes`, `/`).
   - Rendered dashboard at `http://127.0.0.1:5000` via automated browser subagent.
   - Verified search filtering, modal dialogs, and real-time metric counter updates.

---

## 11. Results

Execution of the automated test suite in the target Ubuntu Linux environment produced:

- **C Unit Tests (`test_parser`):** 28 of 28 assertions passed (100% pass rate).
- **Automated Integration Tests (`test_suite.py`):** 35 of 35 test cases passed (100% pass rate).
- **Compilation:** Clean compilation with `gcc -std=c11 -Wall -Wextra -Wpedantic -O2` with zero compiler warnings.
- **Runtime Performance:** Snapshot generation across active `/proc` entries completes in under 15 milliseconds.
- **Browser Compatibility:** Responsive layout rendered accurately on desktop and mobile viewports with active 3-second live polling.

---

## 12. Limitations

1. **Point-in-Time Snapshot Boundary:** A single `/proc` snapshot can only observe current states. It cannot determine if a process with PPID 1 was an orphan or a system service started by init.
2. **Subreaper Nuance:** In desktop Linux environments running modern `systemd --user` session managers, orphaned processes may be reparented to the session subreaper PID rather than PID 1.
3. **Privilege Scope:** Running ProcessScope under a standard non-root user restricts access to certain `/proc/[pid]` attributes belonging to other users. However, basic `/proc/[pid]/stat` remains readable for accessible processes.

---

## 13. Future Enhancements

1. **eBPF Kernel Probing:** Utilize Extended Berkeley Packet Filters (eBPF) or Linux kernel tracepoints (`sched_process_exit`, `sched_process_fork`) to capture continuous lifecycle events, enabling definitive historical orphan tracking.
2. **Process Tree Visualization:** Render an interactive hierarchical SVG tree graph showing parent-child node connections visually in the dashboard.
3. **Configurable Alert Webhooks:** Support notification dispatches (e.g., Slack, email) when zombie counts exceed a configurable threshold over time.

---

## 14. Conclusion

The ProcessScope project successfully implements a comprehensive, robust, and pedagogically sound solution for detecting and understanding zombie and orphan processes in Linux. By combining a safe, space-and-parenthesis resilient C11 scanning engine with controlled demonstration fixtures, a lightweight local Python backend, and an intuitive web dashboard, ProcessScope bridges systems programming principles with modern software observability.
