#!/usr/bin/env python3

import json
import os
import sys
from pathlib import Path


def find_compile_commands(script_dir):
    """Look for compile_commands.json in the script directory."""
    path = os.path.join(script_dir, "compile_commands.json")
    if os.path.isfile(path):
        return path
    return None


def get_clangd_path(compile_commands_path):
    """Extract clang++ path from compile_commands.json and derive clangd path."""
    with open(compile_commands_path, "r") as f:
        data = json.load(f)

    if not data:
        return None

    # Get the command from the first entry
    first_entry = data[0]
    COMMAND = first_entry.get("command", "")

    # Extract the compiler path (first token, or path containing clang++)
    PATH_TO_COMPILER = Path(COMMAND.split(' ')[0])
    CLANGD_PATH = PATH_TO_COMPILER.with_name("clangd")

    # Replace clang++ with clangd
    if CLANGD_PATH.exists() and os.access(CLANGD_PATH, os.X_OK):
        return str(CLANGD_PATH)

    return None


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))

    compile_commands = find_compile_commands(script_dir)
    if not compile_commands:
        print(
            f"ERROR: Could not find compile_commands.json in {script_dir}", file=sys.stderr)
        sys.exit(1)

    clangd_path = get_clangd_path(compile_commands)
    if not clangd_path:
        print(
            f"ERROR: Could not extract clangd path from {compile_commands}", file=sys.stderr)
        sys.exit(1)

    # Execute clangd with all passed arguments
    os.execv(clangd_path, [clangd_path] + sys.argv[1:])


if __name__ == "__main__":
    main()
