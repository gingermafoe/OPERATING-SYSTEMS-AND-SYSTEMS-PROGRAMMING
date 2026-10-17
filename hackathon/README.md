# ProcessScope — Zombie & Orphan Process Detection System

[![Language: C](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Ubuntu Linux](https://img.shields.io/badge/Platform-Ubuntu%20Linux-orange.svg)](https://ubuntu.com/)
[![Web: Vanilla JS](https://img.shields.io/badge/Frontend-Vanilla%20JS%20%2F%20CSS-green.svg)](https://developer.mozilla.org/en-US/docs/Web/JavaScript)
[![Backend: Python 3 Stdlib](https://img.shields.io/badge/Backend-Python%203%20Stdlib-yellow.svg)](https://docs.python.org/3/)
[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

A systems-programming utility and educational monitoring platform for Linux environments. ProcessScope inspects `/proc`, detects defunct **zombie (`Z`)** processes, tracks parent-child process relationships, classifies candidate **adopted processes (PPID 1 / subreaper)**, and provides controlled demonstration fixtures demonstrating process lifecycle transitions.

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [Problem Statement](#problem-statement)
3. [Objectives](#objectives)
4. [Key Features](#key-features)
5. [System Architecture](#system-architecture)
6. [Technologies Used](#technologies-used)
7. [Dependencies & Prerequisites](#dependencies--prerequisites)
8. [Installation & Setup](#installation--setup)
9. [Compilation Commands](#compilation-commands)
10. [Terminal CLI Usage](#terminal-cli-usage)
11. [Web Dashboard Usage](#web-dashboard-usage)
12. [REST API Specification](#rest-api-specification)
13. [Controlled Demonstrations](#controlled-demonstrations)
    - [Zombie Process Lifecycle (`zombie_demo`)](#zombie-process-lifecycle-zombie_demo)
    - [Orphan Process Reparenting (`orphan_demo`)](#orphan-process-reparenting-orphan_demo)
14. [Automated Testing & Validation](#automated-testing--validation)
15. [Expected Output Examples](#expected-output-examples)
16. [Technical Limitations & Heuristics](#technical-limitations--heuristics)
17. [Troubleshooting Guide](#troubleshooting-guide)
18. [GitHub Publishing Guide](#github-publishing-guide)

---

## Project Overview

In POSIX-compliant operating systems, process creation, lifecycle management, and termination form the bedrock of systems stability. Two frequent anomalies arise during process execution:
- **Zombie Processes (`Z` / Defunct):** Processes that have terminated via `exit()`, but whose parent has not yet invoked `wait()` or `waitpid()` to acknowledge their exit status. Although their virtual memory and file handles are released, their **Process Control Block (PCB)** persists in the kernel process table.
- **Orphan Processes:** Processes whose parent terminates while the child is still executing. The Linux kernel reparents orphans to `init` (PID 1) or an ancestor subreaper (`PR_SET_CHILD_SUBREAPER`).

**ProcessScope** provides a non-invasive, read-only monitoring tool written in **C11**, coupled with a lightweight local **Python 3 HTTP server** and a **responsive cybersecurity dashboard** with live `/proc` telemetry.

---

## Problem Statement

System administrators, students, and engineers frequently encounter process table saturation and resource leaks caused by un-reaped child processes. Standard utilities like `ps` and `top` display tabular process dumps, but do not provide:
1. Automated classification distinguishing active processes from defunct zombies and adopted candidates.
2. Conceptual education on the differences between heuristic snapshot inspection and event-proven reparenting.
3. Safe, repeatable, bounded test harnesses to demonstrate these OS primitives in laboratory or evaluation settings without compromising system stability.

---

## Objectives

- **Robust Linux C Scanner:** Parse `/proc/[pid]/stat` safely, accounting for processes with arbitrary spaces and nested parentheses in command names.
- **Strict Error Resilience:** Tolerate processes that exit mid-scan (`ENOENT`) or unreadable entries (`EACCES`) without crashing.
- **Accurate Classification:** Distinguish confirmed zombies (`state == 'Z'`) from snapshot `adopted_candidate` processes (PPID 1 / subreaper).
- **Controlled Demonstrations:** Implement standalone C programs (`zombie_demo` and `orphan_demo`) that demonstrate `fork()`, un-reaped zombies, `waitpid()` resolution, and kernel reparenting.
- **Interactive Web Interface:** Provide real-time auto-refreshing dashboard metrics, sorting, filtering, and process detail modals using standard browser tech (Vanilla HTML/CSS/JS).
- **Zero Third-Party Dependency Backend:** Deliver a secure HTTP API server using only Python's standard library.

---

## Key Features

- **Authoritative C Core:** Compiled with `-std=c11 -Wall -Wextra -Wpedantic -O2`.
- **JSON & Table Output:** Supports formatted ANSI terminal tables and machine-parseable JSON via `--json`.
- **Space & Parenthesis Resistant:** Correctly identifies the `comm` boundary by parsing up to the *last* closing parenthesis `)` in `/proc/[pid]/stat`.
- **Defunct Process Alert:** Prominently flags active zombies awaiting `waitpid()`.
- **Heuristic Awareness:** Explains why snapshot analysis cannot definitively prove orphan history, using the label `adopted_candidate`.
- **Subreaper Awareness:** Documents Linux `prctl(PR_SET_CHILD_SUBREAPER)` reparenting mechanics.
- **Bounded Demo Safety:** Interactive demos feature automatic timeouts and clean termination to prevent stray processes.

---

## System Architecture

```
+-----------------------------------------------------------------------+
|                           ProcessScope Architecture                  |
+-----------------------------------------------------------------------+

   +-----------------------------------------------------------------+
   |                     Linux Operating System                      |
   |                                                                 |
   |   /proc/[pid]/stat  ----->  Linux Virtual Filesystem (procfs)   |
   |   (Kernel Process Table, State 'Z', PPID, Comm)                 |
   +-----------------------------------------------------------------+
              ^                                         ^
              | Read-only scan                          | Controlled fork
              |                                         |
   +----------------------+                 +-------------------------+
   |   bin/processcope    |                 | Controlled Test Fixtures|
   |   (C11 Core Engine)  |                 | - bin/zombie_demo       |
   |   - scanner.c        |                 | - bin/orphan_demo       |
   |   - main.c           |                 +-------------------------+
   +----------------------+
              |
              | stdout JSON (--json)
              v
   +--------------------------------------+
   |    dashboard/server.py               |
   |    (Python 3 Standard Library HTTP)  |
   |    - Binds 127.0.0.1:5000            |
   |    - Safe subprocess execution       |
   |    - Validates JSON payload          |
   +--------------------------------------+
              |
              | HTTP REST / Static Assets
              v
   +--------------------------------------+
   |        Web Browser Dashboard         |
   |        - Dark Navy UI Theme          |
   |        - Real-time Polling (3s)      |
   |        - Metric Counters & Badges    |
   |        - Search, Sort, Filter        |
   +--------------------------------------+
```

---

## Technologies Used

- **Core Engine:** C (C11 standard, POSIX.1-2008)
- **Compiler:** GCC / Clang
- **Build System:** GNU Make
- **Local API Gateway:** Python 3 standard library (`http.server`, `subprocess`, `json`)
- **Frontend:** Vanilla JavaScript (ES6+), CSS3 (Modern dark-navy theme), Semantic HTML5
- **Fonts:** Google Fonts (*Plus Jakarta Sans*, *JetBrains Mono*)

---

## Dependencies & Prerequisites

No third-party packages or external package managers are required. All tooling utilizes standard Ubuntu Linux packages:

```bash
# Verify required system tools
gcc --version
make --version
python3 --version
```

If missing on a clean Ubuntu installation:
```bash
sudo apt update
sudo apt install -y build-essential python3
```

---

## Installation & Setup

Clone or place the repository in your Linux environment or WSL workspace:

```bash
# Clone the repository
git clone https://github.com/your-username/ProcessScope.git
cd ProcessScope
```

---

## Compilation Commands

The included `Makefile` handles dependency tracking, compiler warnings, and target outputs:

```bash
# Clean and compile all binaries into bin/
make clean
make

# Output binaries created in bin/:
#   bin/processcope    (Core scanner executable)
#   bin/zombie_demo    (Controlled zombie demonstration)
#   bin/orphan_demo    (Controlled orphan demonstration)
#   bin/test_parser    (C unit test runner)
```

---

## Terminal CLI Usage

### 1. Full System Process Table Scan
```bash
./bin/processcope
```

### 2. Display Only Zombie Processes
```bash
./bin/processcope --zombies-only
```

### 3. Display Only Candidate Adopted Processes
```bash
./bin/processcope --orphans-only
```

### 4. Inspect a Specific PID
```bash
./bin/processcope --pid 1
```

### 5. Filter by Process Name Substring
```bash
./bin/processcope --name systemd
```

### 6. Emit Machine-Readable JSON
```bash
./bin/processcope --json
```

### 7. View Command Line Options & OS Concepts
```bash
./bin/processcope --help
```

---

## Web Dashboard Usage

Start the local dashboard server:

```bash
# Start server on default port 5000 (binds 127.0.0.1)
make dashboard

# Or launch directly with custom host/port:
python3 dashboard/server.py --host 127.0.0.1 --port 5000
```

Open your browser and navigate to:
```
http://127.0.0.1:5000
```

### Dashboard Capabilities:
- **Telemetry Cards:** Displays total scanned processes, active zombies, candidate adopted processes, and standard processes.
- **Zombie Alert Banner:** Dynamically flashes an alert when one or more zombie processes exist.
- **Instant Search:** Filter table rows in real-time by typing a PID or command name.
- **State & Classification Filters:** Isolate processes by classification (`zombie`, `adopted_candidate`, `standard`) or state (`R`, `S`, `Z`, `D`, `T`).
- **Interactive Inspection Modal:** Click **Inspect** on any row to view its parent-child lineage and technical classification rationale.
- **OS Concepts Modal:** Click **Concepts** in the top navigation bar to access educational summaries of zombie and orphan theory.

---

## REST API Specification

### `GET /api/processes`
Executes `bin/processcope --json` and returns authoritative telemetry.

**Response Schema (`200 OK`):**
```json
{
  "timestamp": 1729141000,
  "total_scanned": 150,
  "counts": {
    "total": 150,
    "zombies": 0,
    "adopted_candidates": 42,
    "standard": 108
  },
  "meta": {
    "note": "Snapshot PPID 1 classification is labeled adopted_candidate because a single snapshot cannot confirm prior parentage; subreapers may also adopt."
  },
  "processes": [
    {
      "pid": 1,
      "ppid": 0,
      "name": "systemd",
      "state": "S",
      "state_desc": "Sleeping / Interruptible (S)",
      "classification": "standard"
    }
  ]
}
```

### `GET /api/health`
Health check verifying server status and presence of compiled binary.

**Response (`200 OK`):**
```json
{
  "status": "healthy",
  "binary_exists": true,
  "binary_path": "/path/to/ProcessScope/bin/processcope",
  "python_version": "3.12.3"
}
```

---

## Controlled Demonstrations

### Zombie Process Lifecycle (`zombie_demo`)

This experiment demonstrates what causes a zombie process and how `waitpid()` resolves it.

1. **Terminal 1 — Launch the Zombie Demo:**
   ```bash
   ./bin/zombie_demo
   ```
   *Action:* The parent forks a child. The child calls `exit(42)`. The parent intentionally pauses without calling `wait()`.

2. **Terminal 2 — Observe the Zombie in ProcessScope:**
   ```bash
   ./bin/processcope --zombies-only
   ```
   *Observation:* The child PID appears with State `Z` and Classification `[ZOMBIE]`. The web dashboard also updates the Zombie count and shows the alert banner.

3. **Terminal 1 — Resolve the Zombie:**
   Press `<Enter>` in Terminal 1.
   *Action:* The parent executes `waitpid(child_pid, &status, 0)` and collects exit status `42`.

4. **Terminal 2 — Verify Dissolution:**
   ```bash
   ./bin/processcope --zombies-only
   ```
   *Observation:* The zombie PID has been removed from the kernel process table.

---

### Orphan Process Reparenting (`orphan_demo`)

This experiment proves how the Linux kernel reparents orphaned processes when their parent terminates prematurely.

1. **Terminal 1 — Run the Orphan Demo:**
   ```bash
   ./bin/orphan_demo
   ```
   *Action:*
   - The original parent creates a child.
   - The parent terminates immediately.
   - The child process calls `getppid()` and prints its new adoptive parent PID.
   - In standard Linux, the child is reparented to `init / systemd (PID 1)` or an active subreaper (e.g. `systemd --user`).
   - The child sleeps for 10 seconds before auto-terminating cleanly.

2. **Terminal 2 — Inspect Child Process:**
   ```bash
   ./bin/processcope --orphans-only
   ```
   *Observation:* The child process is running with PPID `1` (or subreaper PID) and classified as `ADOPTED_CANDIDATE`.

---

## Automated Testing & Validation

Execute the complete automated test suite with:

```bash
make test
```

The test runner runs:
1. **`bin/test_parser`:** Verifies 28 assertions including edge cases:
   - Command names containing spaces (e.g. `Web Content Worker`).
   - Command names containing nested parentheses (e.g. `worker (gpu) thread`).
   - JSON string escaping (quotes, newlines, tabs, backslashes).
   - Safe rejection of malformed or truncated `/proc` records.
2. **`tests/test_suite.py`:**
   - Validates CLI table formatting and summary calculations.
   - Validates JSON output schema against Python `json.loads`.
   - Executes live `zombie_demo`, verifies detection in state `Z`, verifies reaping cleanup.
   - Executes live `orphan_demo`, verifies PPID change.
   - Launches local Python server, verifies `/api/health`, `/api/processes`, and static web assets.

---

## Expected Output Examples

*(Note: PIDs and process names will reflect your active system state.)*

### CLI Table Output Example
```
========================================================================================
  PROCESSSCOPE :: LINUX PROCESS TREE & RELATIONSHIP SCANNER                              
========================================================================================
  PID      PPID     STATE  CLASSIFICATION       COMMAND / NAME                
  --------------------------------------------------------------------------------------
  1        0        S      STANDARD             systemd                       
  2        1        S      ADOPTED_CANDIDATE    init-systemd                  
  50       1        S      ADOPTED_CANDIDATE    systemd-journal               
  99       1        S      ADOPTED_CANDIDATE    systemd-udevd                 
  167      1        S      ADOPTED_CANDIDATE    cron                          
  343      287      S      STANDARD             bash                          
  512      343      Z      ZOMBIE               zombie_child                  
  --------------------------------------------------------------------------------------
  Summary: Total Active: 25 | Zombies (Z): 1 | Adopted Candidates: 13 | Displayed: 7
  [!] ALERT: 1 zombie process(es) detected in defunct state awaiting parent wait().
```

---

## Technical Limitations & Heuristics

1. **Snapshot vs Event Heuristics:**
   A single `/proc` snapshot cannot verify whether a process with `PPID 1` was:
   - A child that became orphaned and was reparented.
   - A legitimate background daemon spawned directly by `systemd`.
   To avoid false claims, ProcessScope labels these entries as `adopted_candidate`. Definite reparenting is verified through the controlled `orphan_demo`.
2. **Subreaper Processes:**
   Modern Linux allows processes to declare themselves subreapers via `prctl(PR_SET_CHILD_SUBREAPER, 1)`. When a process terminates, its orphaned descendants are adopted by the nearest ancestor subreaper rather than PID 1.
3. **Kernel Threads:**
   Kernel threads (e.g. `kthreadd`, `kworker`) are parented by PID 2 and lack traditional user-space memory maps. ProcessScope displays their state (`I` / `S`) based on `/proc/[pid]/stat`.
4. **Permissions:**
   Certain `/proc/[pid]` entries belonging to other users or root may restrict extended attributes. ProcessScope safely handles `EACCES` and continues scanning.

---

## Troubleshooting Guide

| Issue | Cause | Resolution |
| :--- | :--- | :--- |
| `make: gcc: Command not found` | GCC compiler is not installed | Run `sudo apt update && sudo apt install build-essential` |
| `Scanner binary not found` in Dashboard | Project has not been built | Run `make` in the repository root directory |
| `Address already in use: 5000` | Another process is using port 5000 | Run `python3 dashboard/server.py --port 8080` |
| `Failed to read /proc filesystem` | Non-Linux OS or unmounted `/proc` | Execute on native Ubuntu Linux or WSL2 |

---

## GitHub Publishing Guide

To publish this repository cleanly to GitHub:

```bash
# 1. Initialize git repository (if not already done)
git init

# 2. Stage all project files (binaries and objects are excluded via .gitignore)
git add .

# 3. Commit the implementation
git commit -m "Initial commit: ProcessScope Zombie & Orphan Process Detection System"

# 4. Link your GitHub remote
git remote add origin https://github.com/<YOUR_USERNAME>/ProcessScope.git

# 5. Push to GitHub main branch
git branch -M main
git push -u origin main
```

---

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
