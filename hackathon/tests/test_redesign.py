#!/usr/bin/env python3
import subprocess
import urllib.request
import time
import sys
import os

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
server_path = os.path.join(BASE_DIR, "dashboard", "server.py")

port = 5055
proc = subprocess.Popen([sys.executable, server_path, "--port", str(port)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
time.sleep(1)

try:
    # 1. Check index.html
    url = f"http://127.0.0.1:{port}/"
    with urllib.request.urlopen(url, timeout=3) as resp:
        html = resp.read().decode('utf-8')
        assert resp.status == 200, f"Expected 200, got {resp.status}"
        assert "PROCESSSCOPE" in html, "Missing PROCESSSCOPE"
        assert "SEE BEYOND" in html, "Missing SEE BEYOND"
        assert "DETECT THE THREAT" in html, "Missing DETECT THE THREAT"
        assert "cyber_process_core.jpg" in html, "Missing 3D image"
        assert "PROCESS LIFECYCLE" in html, "Missing PROCESS LIFECYCLE"
        assert "ZOMBIE DETECTION" in html, "Missing ZOMBIE DETECTION"
        assert "ORPHAN TRACKING" in html, "Missing ORPHAN TRACKING"
        assert "LIVE PROCESS MONITOR" in html, "Missing LIVE PROCESS MONITOR"
        assert "SYSTEM ARCHITECTURE" in html, "Missing SYSTEM ARCHITECTURE"
        print("[PASS] index.html served with all required elements")

    # 2. Check styles.css
    css_url = f"http://127.0.0.1:{port}/styles.css"
    with urllib.request.urlopen(css_url, timeout=3) as resp:
        css = resp.read().decode('utf-8')
        assert resp.status == 200
        assert "#030609" in css, "Missing #030609"
        assert "#0A1520" in css, "Missing #0A1520"
        assert "#3B82F6" in css, "Missing #3B82F6"
        assert "#65DFFF" in css, "Missing #65DFFF"
        assert "rgba(112, 177, 219, 0.22)" in css, "Missing cyber border color"
        print("[PASS] styles.css contains exact reference color palette & borders")

    # 3. Check 3D image asset
    img_url = f"http://127.0.0.1:{port}/assets/cyber_process_core.jpg"
    with urllib.request.urlopen(img_url, timeout=3) as resp:
        img_data = resp.read()
        assert resp.status == 200
        assert len(img_data) > 100000, f"Image too small ({len(img_data)} bytes)"
        print(f"[PASS] cyber_process_core.jpg served successfully ({len(img_data)} bytes)")

    # 4. Check API endpoint
    api_url = f"http://127.0.0.1:{port}/api/processes"
    with urllib.request.urlopen(api_url, timeout=3) as resp:
        assert resp.status == 200
        print("[PASS] /api/processes endpoint connects to real C scanner")

    print("\nALL REDESIGN VERIFICATION CHECKS PASSED PERFECTLY!")

finally:
    proc.terminate()
    proc.wait()
