#!/usr/bin/env python3

# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Smart clangd proxy that auto-restarts when switching between project areas.

This script acts as a proxy between VSCode and clangd, monitoring which files
are being opened. When a file from a different project area (with its own
compile_commands.json) is opened, it automatically restarts clangd with the
correct binary for that area.

This solves the BMI compatibility issue when working with C++20 modules across
multiple subprojects that may use different LLVM toolchain versions.

Usage:
    Set as clangd.path in VSCode settings:
    "clangd.path": "${workspaceFolder}/compile_commands_clangd_resolver.py"
"""

import json
import os
import signal
import subprocess
import sys
import threading
from pathlib import Path
from typing import Optional


class ClangdProxy:
    def __init__(self, script_dir: str, args: list[str]):
        self.script_dir = script_dir
        self.args = args
        self.current_clangd_path: Optional[str] = None
        self.clangd_process: Optional[subprocess.Popen] = None
        self.running = True
        self.lock = threading.Lock()

    def find_compile_commands(self, file_path: str) -> Optional[str]:
        """Search upward from file_path to find the nearest compile_commands.json."""
        path = Path(file_path).resolve()

        # Start from the file's directory
        current = path.parent if path.is_file() else path

        while current != current.parent:
            candidate = current / "compile_commands.json"
            if candidate.is_file():
                return str(candidate)
            current = current.parent

        return None

    def get_clangd_from_compile_commands(self, compile_commands_path: str) -> Optional[str]:
        """Extract clangd path from compile_commands.json."""
        try:
            with open(compile_commands_path, "r") as f:
                data = json.load(f)

            if not data:
                return None

            first_entry = data[0]
            command = first_entry.get("command", "")
            if not command:
                return None

            compiler_path = Path(command.split(' ')[0])
            clangd_path = compiler_path.with_name("clangd")

            if clangd_path.exists() and os.access(clangd_path, os.X_OK):
                return str(clangd_path)

        except (json.JSONDecodeError, IOError, IndexError):
            pass

        return None

    def start_clangd(self, clangd_path: str) -> bool:
        """Start a new clangd process."""
        with self.lock:
            self.stop_clangd()

            try:
                self.clangd_process = subprocess.Popen(
                    [clangd_path] + self.args,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=sys.stderr,
                )
                self.current_clangd_path = clangd_path
                print(f"[clangd-proxy] Started clangd: {clangd_path}", file=sys.stderr)
                return True
            except OSError as e:
                print(f"[clangd-proxy] Failed to start clangd: {e}", file=sys.stderr)
                return False

    def stop_clangd(self):
        """Stop the current clangd process."""
        if self.clangd_process:
            try:
                self.clangd_process.terminate()
                self.clangd_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.clangd_process.kill()
                self.clangd_process.wait()
            except Exception:
                pass
            self.clangd_process = None
            self.current_clangd_path = None

    def extract_file_uri_from_message(self, msg: dict) -> Optional[str]:
        """Extract file URI from LSP message if present."""
        # Check textDocument/didOpen
        if msg.get("method") == "textDocument/didOpen":
            params = msg.get("params", {})
            text_doc = params.get("textDocument", {})
            uri = text_doc.get("uri", "")
            if uri.startswith("file://"):
                return uri[7:]  # Remove file:// prefix

        # Check textDocument/didSave, textDocument/didChange
        if msg.get("method") in ["textDocument/didSave", "textDocument/didChange"]:
            params = msg.get("params", {})
            text_doc = params.get("textDocument", {})
            uri = text_doc.get("uri", "")
            if uri.startswith("file://"):
                return uri[7:]

        return None

    def maybe_switch_clangd(self, file_path: str):
        """Check if we need to switch to a different clangd for this file."""
        compile_commands = self.find_compile_commands(file_path)
        if not compile_commands:
            return

        new_clangd = self.get_clangd_from_compile_commands(compile_commands)
        if not new_clangd:
            return

        if new_clangd != self.current_clangd_path:
            print(f"[clangd-proxy] Switching clangd for: {file_path}", file=sys.stderr)
            print(f"[clangd-proxy] compile_commands.json: {compile_commands}", file=sys.stderr)
            self.start_clangd(new_clangd)

    def read_lsp_message(self, stream) -> Optional[bytes]:
        """Read a complete LSP message from stream."""
        headers = {}
        while True:
            line = stream.readline()
            if not line:
                return None
            line = line.decode('utf-8').strip()
            if not line:
                break
            if ':' in line:
                key, value = line.split(':', 1)
                headers[key.strip().lower()] = value.strip()

        content_length = int(headers.get('content-length', 0))
        if content_length == 0:
            return None

        content = stream.read(content_length)
        return content

    def write_lsp_message(self, stream, content: bytes):
        """Write a complete LSP message to stream."""
        header = f"Content-Length: {len(content)}\r\n\r\n"
        stream.write(header.encode('utf-8'))
        stream.write(content)
        stream.flush()

    def forward_stdout(self):
        """Forward clangd stdout to our stdout."""
        while self.running:
            with self.lock:
                process = self.clangd_process

            if not process or not process.stdout:
                break

            try:
                content = self.read_lsp_message(process.stdout)
                if content:
                    self.write_lsp_message(sys.stdout.buffer, content)
                else:
                    # Process ended, wait a bit and check if restarted
                    import time
                    time.sleep(0.1)
            except Exception:
                break

    def run(self):
        """Main proxy loop."""
        # Start with the script directory's compile_commands.json
        initial_compile_commands = os.path.join(self.script_dir, "compile_commands.json")
        initial_clangd = None

        if os.path.isfile(initial_compile_commands):
            initial_clangd = self.get_clangd_from_compile_commands(initial_compile_commands)

        if not initial_clangd:
            print(f"[clangd-proxy] ERROR: No initial clangd found in {self.script_dir}", file=sys.stderr)
            sys.exit(1)

        if not self.start_clangd(initial_clangd):
            sys.exit(1)

        # Start stdout forwarding thread
        stdout_thread = threading.Thread(target=self.forward_stdout, daemon=True)
        stdout_thread.start()

        # Main loop: read from stdin, inspect, forward to clangd
        while self.running:
            try:
                content = self.read_lsp_message(sys.stdin.buffer)
                if not content:
                    break

                # Parse and inspect the message
                try:
                    msg = json.loads(content.decode('utf-8'))
                    file_path = self.extract_file_uri_from_message(msg)
                    if file_path:
                        self.maybe_switch_clangd(file_path)
                except json.JSONDecodeError:
                    pass

                # Forward to clangd
                with self.lock:
                    if self.clangd_process and self.clangd_process.stdin:
                        self.write_lsp_message(self.clangd_process.stdin, content)

            except Exception as e:
                print(f"[clangd-proxy] Error: {e}", file=sys.stderr)
                break

        self.running = False
        self.stop_clangd()


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    args = sys.argv[1:]

    # Handle signals gracefully
    proxy = ClangdProxy(script_dir, args)

    def signal_handler(signum, frame):
        proxy.running = False
        proxy.stop_clangd()
        sys.exit(0)

    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    proxy.run()


if __name__ == "__main__":
    main()
