#!/usr/bin/env python3
"""
ProcessScope Dashboard Server
Course: Operating Systems and Systems Programming

A lightweight, secure HTTP server built exclusively with the Python 3 standard library.
Serves the web dashboard and provides a read-only REST API by safely executing
the compiled C scanner binary ('bin/processcope --json').
"""

import sys
import os
import json
import subprocess
import argparse
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

# Resolve project root directory
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
STATIC_DIR = os.path.join(BASE_DIR, "dashboard", "static")
BINARY_PATH = os.path.join(BASE_DIR, "bin", "processcope")


def get_process_data():
    """
    Safely executes the compiled C scanner binary without invoking a shell.
    Validates and returns the parsed JSON payload.
    """
    if not os.path.exists(BINARY_PATH):
        return {
            "error": "Scanner binary not found. Please compile using 'make' first.",
            "binary_path": BINARY_PATH
        }, 503

    try:
        # Run binary directly with arguments, capturing stdout safely
        result = subprocess.run(
            [BINARY_PATH, "--json"],
            capture_output=True,
            text=True,
            timeout=5.0,
            check=False
        )

        if result.returncode != 0:
            return {
                "error": "ProcessScope scanner exited with non-zero status.",
                "returncode": result.returncode,
                "stderr": result.stderr.strip()
            }, 500

        data = json.loads(result.stdout)
        return data, 200

    except subprocess.TimeoutExpired:
        return {"error": "ProcessScope scan timed out after 5.0 seconds."}, 504
    except json.JSONDecodeError as e:
        return {
            "error": "Failed to parse JSON emitted by scanner binary.",
            "details": str(e)
        }, 500
    except Exception as e:
        return {"error": f"Internal server error: {str(e)}"}, 500


class ProcessScopeHandler(BaseHTTPRequestHandler):
    """
    Custom HTTP request handler serving static dashboard assets and JSON API.
    """

    server_version = "ProcessScope-HTTP/1.0"

    def _set_headers(self, status=200, content_type="application/json; charset=utf-8"):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # API Endpoints
        if path == "/api/processes":
            data, status = get_process_data()
            self._set_headers(status, "application/json; charset=utf-8")
            self.wfile.write(json.dumps(data, indent=2).encode("utf-8"))
            return

        elif path == "/api/health":
            health = {
                "status": "healthy",
                "binary_exists": os.path.exists(BINARY_PATH),
                "binary_path": BINARY_PATH,
                "python_version": sys.version.split()[0]
            }
            self._set_headers(200, "application/json; charset=utf-8")
            self.wfile.write(json.dumps(health, indent=2).encode("utf-8"))
            return

        # Static File Serving
        if path in ("/", "/index.html"):
            file_path = os.path.join(STATIC_DIR, "index.html")
            content_type = "text/html; charset=utf-8"
        elif path == "/styles.css":
            file_path = os.path.join(STATIC_DIR, "styles.css")
            content_type = "text/css; charset=utf-8"
        elif path == "/app.js":
            file_path = os.path.join(STATIC_DIR, "app.js")
            content_type = "application/javascript; charset=utf-8"
        else:
            self._set_headers(404, "application/json; charset=utf-8")
            self.wfile.write(json.dumps({"error": "Not Found"}).encode("utf-8"))
            return

        if not os.path.isfile(file_path):
            self._set_headers(404, "application/json; charset=utf-8")
            self.wfile.write(json.dumps({"error": "File not found"}).encode("utf-8"))
            return

        try:
            with open(file_path, "rb") as f:
                content = f.read()
            self._set_headers(200, content_type)
            self.wfile.write(content)
        except Exception as e:
            self._set_headers(500, "application/json; charset=utf-8")
            self.wfile.write(json.dumps({"error": f"Failed reading file: {str(e)}"}).encode("utf-8"))

    def log_message(self, format, *args):
        # Format clean log lines to stderr
        sys.stderr.write(f"[{self.log_date_time_string()}] {format % args}\n")


def run(host="127.0.0.1", port=5000):
    server_address = (host, port)
    httpd = HTTPServer(server_address, ProcessScopeHandler)
    print("======================================================================")
    print("  PROCESSSCOPE :: SYSTEM MONITORING & PROCESS DASHBOARD SERVER       ")
    print("======================================================================")
    print(f"  Binding Address  : http://{host}:{port}")
    print(f"  Process Data API : http://{host}:{port}/api/processes")
    print(f"  Health Check     : http://{host}:{port}/api/health")
    print(f"  Authoritative C  : {BINARY_PATH}")
    print("----------------------------------------------------------------------")
    print("  Press Ctrl+C to gracefully stop the server.\n")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[Server] Shutting down gracefully...")
        httpd.server_close()
        print("[Server] Stopped.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ProcessScope Web Dashboard Server")
    parser.add_argument("--host", default="127.0.0.1", help="Host address (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=5000, help="Port number (default: 5000)")
    args = parser.parse_args()
    run(host=args.host, port=args.port)
