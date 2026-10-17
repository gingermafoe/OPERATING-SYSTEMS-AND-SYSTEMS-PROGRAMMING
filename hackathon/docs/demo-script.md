# ProcessScope: Live Demonstration Script & Evaluator Guide

This step-by-step guide is designed for a student or presenter delivering a live demonstration of **ProcessScope** during an Operating Systems lab evaluation or hackathon presentation.

---

## 1. Environment Preparation & Compilation

### Step 1.1: Open Terminal 1
Ensure you are in the project root directory:

```bash
cd ProcessScope
```

### Step 1.2: Compile All Binaries
Run GNU Make to compile the C scanner, demonstration fixtures, and unit test runner:

```bash
make clean
make
```

**Expected Output:**
```
rm -rf obj bin
mkdir -p obj
cc -std=c11 -Wall -Wextra -Wpedantic -O2 ... -c src/main.c -o obj/main.o
cc -std=c11 -Wall -Wextra -Wpedantic -O2 ... -c src/scanner.c -o obj/scanner.o
mkdir -p bin
cc ... obj/main.o obj/scanner.o -o bin/processcope
cc ... tests/zombie_demo.c -o bin/zombie_demo
cc ... tests/orphan_demo.c -o bin/orphan_demo
cc ... tests/test_parser.c obj/scanner.o -o bin/test_parser
```
*Note for Evaluator:* Compilation finishes with **zero warnings** under `-Wall -Wextra -Wpedantic`.

---

## 2. Automated Test Suite Demonstration

Run the automated test suite to prove system correctness before interactive testing:

```bash
make test
```

**What the Evaluator Observes:**
- 28 C unit assertions verify stat line parsing with spaces, nested parentheses, and JSON escaping.
- 7 integration test suites execute CLI checks, JSON schema checks, a live zombie creation/reap cycle, an orphan reparenting cycle, and REST API tests.
- Final summary displays: `35 PASSED | 0 FAILED`.

---

## 3. Terminal Scanner Demonstration

### Step 3.1: Full Process Table Scan
```bash
./bin/processcope
```
**Observation:**
- Clean ANSI-formatted table displaying PID, PPID, State, Classification, and Command Name.
- Summary bar displaying active process count and zombie count.

### Step 3.2: Filter by Name or State
```bash
./bin/processcope --name systemd
./bin/processcope --orphans-only
```
**Observation:** Only matching processes are filtered and displayed.

### Step 3.3: Machine-Readable JSON Output
```bash
./bin/processcope --json | head -n 25
```
**Observation:** Valid JSON emitted directly to stdout, ready for pipeline ingestion.

---

## 4. Web Dashboard Demonstration

### Step 4.1: Launch Dashboard Server
In Terminal 1, run:

```bash
make dashboard
```

**Expected Output:**
```
======================================================================
  PROCESSSCOPE :: SYSTEM MONITORING & PROCESS DASHBOARD SERVER       
======================================================================
  Binding Address  : http://127.0.0.1:5000
  Process Data API : http://127.0.0.1:5000/api/processes
  Health Check     : http://127.0.0.1:5000/api/health
  Authoritative C  : /path/to/ProcessScope/bin/processcope
----------------------------------------------------------------------
```

### Step 4.2: Open Browser
Open your browser and navigate to:
```
http://127.0.0.1:5000
```

**What the Evaluator Observes:**
1. **Dark Navy Interface:** Professional systems-engineering aesthetic with telemetry cards (Total, Zombies, Adopted Candidates, Standard).
2. **Real-time Live Indicator:** Green pulsing dot indicating live connection to the C backend with 3-second auto-polling.
3. **Interactive Search:** Type a process name (e.g., `bash` or `systemd`) to watch the table instantly filter rows.
4. **Inspect Modal:** Click **Inspect** on any row to display technical details explaining the process classification.
5. **Concepts Modal:** Click **Concepts** in the top navigation bar to display an OS theory summary.

---

## 5. Controlled Zombie Process Demonstration

Keep the dashboard open in your browser, or open a second terminal (**Terminal 2**).

### Step 5.1: In Terminal 2, Launch the Zombie Demo
```bash
./bin/zombie_demo
```

**What Happens in Terminal 2:**
```
======================================================================
  PROCESSSCOPE :: CONTROLLED ZOMBIE PROCESS DEMONSTRATION             
======================================================================
[Init] Parent Process running with PID: 12345
[Init] Forking child process...

  [Child  PID 12346] Created by Parent 12345.
  [Child  PID 12346] Executing termination via exit(42)...
[Parent PID 12345] Child process (PID 12346) has called exit().
[Parent PID 12345] Child is now in STATE 'Z' (Zombie / Defunct) because
                the parent has NOT yet called wait() or waitpid().

>>> OBSERVATION STEP <<<
Press [ENTER] to reap the zombie child with waitpid(), or wait for timeout:
```

### Step 5.2: In Terminal 3 or on the Web Dashboard, Observe the Zombie
- **On the Web Dashboard:**
  - Notice the **Red Alert Banner** instantly appears: *"1 Active Zombie Process Detected!"*.
  - The **Zombie Processes (Z)** metric card updates to `1` in bright red.
  - The process appears in the table with a red `[ZOMBIE]` badge.
- **In Terminal 3:**
  ```bash
  ./bin/processcope --zombies-only
  ```
  *Output displays the child PID with State `Z` and Classification `[ZOMBIE]`.*

### Step 5.3: Resolve the Zombie in Terminal 2
Switch back to Terminal 2 and press `<Enter>`.

**What Happens in Terminal 2:**
```
[Parent PID 12345] Calling waitpid(12346, &status, 0)...
[Parent PID 12345] Successfully reaped child PID 12346!
[Parent PID 12345] Child exit code acknowledged: 42

[Cleanup] The PCB entry has been freed from the Linux kernel process table.
```

### Step 5.4: Verify Dissolution
- **On the Web Dashboard:** Within 3 seconds, the red banner disappears, the zombie card returns to `0`, and the table row is cleared.
- **In Terminal 3:**
  ```bash
  ./bin/processcope --zombies-only
  ```
  *Returns `Zombies (Z): 0`.*

---

## 6. Controlled Orphan Process Demonstration

### Step 6.1: In Terminal 2, Run the Orphan Demo
```bash
./bin/orphan_demo
```

**What Happens in Terminal 2:**
```
======================================================================
  PROCESSSCOPE :: CONTROLLED ORPHAN REPARENTING DEMONSTRATION         
======================================================================
[Init] Original Parent Process PID: 21000
[Init] Forking child process...

[Parent PID 21000] Child process created with PID 21001.
[Parent PID 21000] Parent is exiting now to orphan the child...
[Parent PID 21000] Terminated.

  [Child Process PID 21001] =============================================
  [Child Process PID 21001] Original Parent PID was: 21000
  [Child Process PID 21001] New Adoptive Parent PID is: 1
  [Reparenting Confirmed] Child was adopted directly by init / systemd (PID 1)!
  >>> OBSERVATION WINDOW (10 SECONDS) <<<
```

### Step 6.2: Observe the Child in ProcessScope
While the child is in its 10-second observation window:
- In Terminal 3:
  ```bash
  ./bin/processcope --pid 21001
  ```
- Or search `21001` in the Web Dashboard.
- **Observation:** Notice that the child's PPID has changed to `1` (or the desktop session subreaper PID) and is categorized as `ADOPTED_CANDIDATE`.
- After 10 seconds, the child automatically terminates safely without leaving any orphaned background processes on the machine.

---

## 7. Explanation of Major Functions (Quick Reference)

| Function / File | Purpose & How to Explain to Evaluator |
| :--- | :--- |
| `parse_stat_line()` in `scanner.c` | Extracts PID, name, state, and PPID from `/proc/[pid]/stat`. Explains how finding the *last* `)` avoids parsing bugs when process names contain spaces or parentheses. |
| `scan_processes()` in `scanner.c` | Iterates numeric directories in `/proc`, safely skips disappearing entries (`ENOENT`), and dynamically populates the `ProcessSnapshot` struct. |
| `classify_process()` in `scanner.c` | Applies OS classification: assigns `CLASS_ZOMBIE` if state is `Z`; assigns `CLASS_ADOPTED_CANDIDATE` if PPID is 1 (explaining snapshot limitations). |
| `print_json()` in `scanner.c` | Serializes snapshot data with proper JSON character escaping for quotes, backslashes, and control characters. |
| `zombie_demo.c` | Creates an intentional un-reaped child using `fork()` and `exit()`, proving that PCB persists in state `Z` until `waitpid()` is executed. |
| `orphan_demo.c` | Demonstrates kernel reparenting by having the parent exit while the child continues executing, then auto-terminates after a bounded timer. |
| `server.py` | Secure local HTTP gateway using Python standard library; invokes C scanner safely via `subprocess.run()` without shell interpretation. |

---

## 8. Common Questions & Evaluator Discussion Points

**Q1: Why is PPID 1 called "adopted_candidate" instead of definitively "orphan"?**  
*Answer:* A single `/proc` snapshot cannot inspect past history. Many processes (such as `systemd-journald`, `dbus-daemon`, `cron`) are spawned directly by PID 1 and are not orphans. Our tool honestly reflects this limitation, while our controlled `orphan_demo` verifies real reparenting dynamically.

**Q2: What is a Subreaper?**  
*Answer:* In Linux 3.4+, a process can call `prctl(PR_SET_CHILD_SUBREAPER, 1)`. When a descendant's parent dies, the child is reparented to the nearest ancestor subreaper (such as `systemd --user`) rather than PID 1. ProcessScope correctly documents this behavior.

**Q3: Why doesn't a zombie consume CPU or RAM?**  
*Answer:* When `exit()` is called, the kernel immediately frees the virtual memory pages, code segment, and file descriptors. Only the small PCB struct (~few kilobytes) remains in the kernel process table to hold the termination status.

**Q4: Why are zombies dangerous if they don't use RAM?**  
*Answer:* Because the Linux kernel has a maximum PID limit (typically 32,768, defined in `/proc/sys/kernel/pid_max`). If zombies accumulate unchecked, all PIDs are occupied, and no new processes can be created.
