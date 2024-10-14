#!/usr/bin/env python3
"""Run a tcpreplay tool and diff its output against a committed reference file.

This mirrors the diff-based checks in test/Makefile.am for the meson test
suite: the tool is executed and, on success, its generated output file is
compared byte-for-byte against the expected reference file.

Usage: run_compare.py [--diff DIFF] [--stdout] <expected> <output> <prog> [args...]

With --stdout, the program's standard output is captured into <output>
(for tools like "tcpprep -P/-I" that print to stdout rather than writing a
file via -o).
"""
import subprocess
import sys


def main():
    """Run the tool, then diff its output against the reference file."""
    argv = sys.argv[1:]
    diff = "diff"
    capture_stdout = False
    while argv:
        if argv[0] == "--diff":
            if len(argv) < 2:
                sys.stderr.write("--diff requires an argument\n")
                return 2
            diff = argv[1]
            argv = argv[2:]
        elif argv[0].startswith("--diff="):
            diff = argv[0][len("--diff=") :]
            argv = argv[1:]
        elif argv[0] == "--stdout":
            capture_stdout = True
            argv = argv[1:]
        else:
            break

    if len(argv) < 3:
        sys.stderr.write(
            "usage: run_compare.py [--diff DIFF] [--stdout] <expected> "
            "<output> <prog> [args...]\n"
        )
        return 2

    expected = argv[0]
    output = argv[1]
    cmd = argv[2:]

    if capture_stdout:
        with open(output, "wb") as out:
            proc = subprocess.run(cmd, check=False, stdout=out)
    else:
        proc = subprocess.run(cmd, check=False)
    if proc.returncode != 0:
        sys.stderr.write(f"command exited {proc.returncode}: {' '.join(cmd)}\n")
        return proc.returncode

    return subprocess.run([diff, expected, output], check=False).returncode


if __name__ == "__main__":
    sys.exit(main())
