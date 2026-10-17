#!/usr/bin/env python3
"""
ProcessScope Automated Integration & Validation Test Suite
Course: Operating Systems and Systems Programming

Executes end-to-end checks across:
  1. C Unit Tests (test_parser)
  2. CLI Terminal Output
  3. JSON Output Schema & Serialization
  4. Controlled Zombie Creation & Reaping Lifecycle
  5. Controlled Orphan Reparenting Observation
  6. Python HTTP API & Static File Serving
"""

import sys
import os
import json
import time
import urllib.request
import subprocess

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BIN_DIR = os.path.join(BASE_DIR, "bin")

PROCESSCOPE_BIN = os.path.join(BIN_DIR, "processcope")
ZOMBIE_DEMO_BIN = os.path.join(BIN_DIR, "zombie_demo")
ORPHAN_DEMO_BIN = os.path.join(BIN_DIR, "orphan_demo")
TEST_PARSER_BIN = os.path.join(BIN_DIR, "test_parser")
SERVER_PY = os.path.join(BASE_DIR, "dashboard", "server.py")

passed = 0
failed = 0


def log_test(name, success, detail=""):
    global passed, failed
    if success:
        passed += 1
        print(f"  [PASS] {name}")
    else:
        failed += 1
        print(f"  [FAIL] {name}: {detail}")


def test_binaries_exist():
    print("\n--- Test Suite 1: Binary Existence Check ---")
    for b in [PROCESSCOPE_BIN, ZOMBIE_DEMO_BIN, ORPHAN_DEMO_BIN, TEST_PARSER_BIN]:
        exists = os.path.isfile(b) and os.access(b, os.X_OK)
        log_test(f"Binary executable: {os.path.basename(b)}", exists)


def test_c_unit_parser():
    print("\n--- Test Suite 2: C Parser Unit Tests ---")
    res = subprocess.run([TEST_PARSER_BIN], capture_output=True, text=True)
    log_test("test_parser exit code 0", res.returncode == 0, f"code {res.returncode}")
    log_test("test_parser 28/28 assertions", "28 of 28 assertions passed" in res.stdout)


def test_cli_terminal_output():
    print("\n--- Test Suite 3: CLI Terminal Output Format ---")
    res = subprocess.run([PROCESSCOPE_BIN], capture_output=True, text=True)
    log_test("CLI scan exit code 0", res.returncode == 0)
    log_test("Contains table header", "PROCESSSCOPE :: LINUX PROCESS TREE" in res.stdout)
    log_test("Contains column titles", "PID" in res.stdout and "PPID" in res.stdout and "STATE" in res.stdout)
    log_test("Contains summary counts", "Summary: Total Active:" in res.stdout)


def test_json_schema():
    print("\n--- Test Suite 4: JSON Output Schema Validation ---")
    res = subprocess.run([PROCESSCOPE_BIN, "--json"], capture_output=True, text=True)
    log_test("JSON mode exit code 0", res.returncode == 0)

    try:
        data = json.loads(res.stdout)
        log_test("JSON is valid syntax", True)
    except Exception as e:
        log_test("JSON is valid syntax", False, str(e))
        return

    log_test("Contains 'timestamp' field", "timestamp" in data)
    log_test("Contains 'counts' object", "counts" in data and isinstance(data["counts"], dict))
    log_test("Counts contain total, zombies, adopted_candidates", 
             all(k in data["counts"] for k in ("total", "zombies", "adopted_candidates")))
    log_test("Contains 'processes' array", "processes" in data and isinstance(data["processes"], list))

    if data["processes"]:
        p = data["processes"][0]
        valid_keys = all(k in p for k in ("pid", "ppid", "name", "state", "classification"))
        log_test("Process record has all required fields", valid_keys)


def test_zombie_lifecycle():
    print("\n--- Test Suite 5: Controlled Zombie Lifecycle & Resolution ---")
    # Launch zombie_demo with 15-second safety timeout
    proc = subprocess.Popen(
        [ZOMBIE_DEMO_BIN, "15"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    # Allow time for child to fork and exit into state Z
    time.sleep(0.3)

    # Scan for zombies
    res = subprocess.run([PROCESSCOPE_BIN, "--json"], capture_output=True, text=True)
    try:
        data = json.loads(res.stdout)
        zombies = [p for p in data.get("processes", []) if p["classification"] == "zombie"]
        log_test("Zombie detected while parent delays wait()", len(zombies) >= 1)
        zombie_pids = [p["pid"] for p in zombies]
    except Exception as e:
        log_test("Zombie detected while parent delays wait()", False, str(e))
        zombie_pids = []

    # Send Enter key to parent to trigger waitpid() reaping
    stdout_out, _ = proc.communicate(input="\n", timeout=5)
    log_test("zombie_demo terminated cleanly", proc.returncode == 0)
    log_test("Parent called waitpid() successfully", "Successfully reaped child" in stdout_out)

    # Verify that the reaped zombie is now gone from /proc
    time.sleep(0.1)
    res_after = subprocess.run([PROCESSCOPE_BIN, "--json"], capture_output=True, text=True)
    try:
        data_after = json.loads(res_after.stdout)
        remaining_pids = {p["pid"] for p in data_after.get("processes", [])}
        reaped_gone = all(zpid not in remaining_pids for zpid in zombie_pids)
        log_test("Reaped child PID dissolved from process table", reaped_gone)
    except Exception as e:
        log_test("Reaped child PID dissolved from process table", False, str(e))


def test_orphan_reparenting():
    print("\n--- Test Suite 6: Controlled Orphan Reparenting Verification ---")
    # Run orphan demo for 2 seconds duration
    res = subprocess.run([ORPHAN_DEMO_BIN, "2"], capture_output=True, text=True, timeout=8)
    log_test("orphan_demo executed successfully", res.returncode == 0)
    log_test("Original parent reported", "Original Parent PID was:" in res.stdout)
    log_test("New adoptive parent reported", "New Adoptive Parent PID is:" in res.stdout)
    log_test("Reparenting confirmed by kernel", "Reparenting Confirmed" in res.stdout)
    log_test("Child auto-terminated safely", "Bounded demo duration complete" in res.stdout)


def test_dashboard_api():
    print("\n--- Test Suite 7: Python Dashboard Server & REST API ---")
    port = 5928
    server_proc = subprocess.Popen(
        [sys.executable, SERVER_PY, "--port", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    # Give server time to bind socket
    time.sleep(0.8)

    try:
        # Test 1: Health endpoint
        health_url = f"http://127.0.0.1:{port}/api/health"
        with urllib.request.urlopen(health_url, timeout=3) as resp:
            log_test("GET /api/health HTTP 200", resp.status == 200)
            data = json.loads(resp.read().decode("utf-8"))
            log_test("Health status is 'healthy'", data.get("status") == "healthy")
            log_test("Health confirms binary exists", data.get("binary_exists") is True)

        # Test 2: Processes API endpoint
        proc_url = f"http://127.0.0.1:{port}/api/processes"
        with urllib.request.urlopen(proc_url, timeout=3) as resp:
            log_test("GET /api/processes HTTP 200", resp.status == 200)
            data = json.loads(resp.read().decode("utf-8"))
            log_test("Processes API returns valid counts", "counts" in data)
            log_test("Processes API returns process array", isinstance(data.get("processes"), list))

        # Test 3: Static index.html endpoint
        index_url = f"http://127.0.0.1:{port}/"
        with urllib.request.urlopen(index_url, timeout=3) as resp:
            log_test("GET / serves index.html HTTP 200", resp.status == 200)
            html = resp.read().decode("utf-8")
            log_test("HTML contains ProcessScope title", "ProcessScope" in html)

    except Exception as e:
        log_test("API test execution", False, str(e))
    finally:
        server_proc.terminate()
        server_proc.wait(timeout=3)
        log_test("Server stopped cleanly", True)


def main():
    print("======================================================================")
    print("  PROCESSSCOPE :: AUTOMATED TEST & VERIFICATION SUITE                 ")
    print("======================================================================")

    test_binaries_exist()
    test_c_unit_parser()
    test_cli_terminal_output()
    test_json_schema()
    test_zombie_lifecycle()
    test_orphan_reparenting()
    test_dashboard_api()

    print("\n======================================================================")
    print(f"  FINAL SUMMARY: {passed} PASSED | {failed} FAILED")
    print("======================================================================\n")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
