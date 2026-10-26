#!/usr/bin/env python3
"""Black-box test runner for the C-like language compiler.

The compiler is treated as a black box: only its process exit code is
observed.  Compiler stdout and stderr are discarded.

Directory contract (relative to --tests-dir):

    pass/semantic/  parse-only: 0,  full: 0
    fail/semantic/  parse-only: 0,  full: != 0
    fail/syntax/    parse-only: != 0
    pass/syntax/    parse-only: 0  (full mode not asserted)

Mode semantics:

    full    runs every directory above.
    syntax  runs only pass/syntax and fail/syntax (parse-only invocation).
            Both semantic directories are skipped and counted as SKIP.

Skipped files are not reported per-file unless --filter was given; they
are still counted in the summary.

Usage:
    python3 run_tests.py --compiler PATH [--mode {syntax,full}]
                         [--timeout N] [--filter REGEX] [--tests-dir DIR]
"""

import argparse
import os
import re
import signal
import subprocess
import sys
import tempfile

PARSE_ONLY = "--syntax"

EXIT_OK = 0
EXIT_TEST_FAILURE = 1
EXIT_CONFIG_ERROR = 2

VALID_LAYOUTS = {
    ("pass", "semantic"),
    ("pass", "syntax"),
    ("fail", "semantic"),
    ("fail", "syntax"),
}


def build_arg_parser():
    parser = argparse.ArgumentParser(
        prog="run_tests.py",
        description="Run the black-box compiler test suite.",
    )
    parser.add_argument(
        "--compiler",
        required=True,
        help="path to the compiler executable",
    )
    parser.add_argument(
        "--mode",
        choices=("syntax", "full"),
        default="full",
        help="which expectations to verify (default: full)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="per-invocation timeout in seconds (default: 10)",
    )
    parser.add_argument(
        "--filter",
        default=None,
        help="only run test files whose path matches this regex",
    )
    parser.add_argument(
        "--tests-dir",
        default="tests/frontend/inputs",
        help="root directory of the test corpus (default: tests/frontend/inputs)",
    )
    return parser


def is_executable(path):
    return os.path.isfile(path) and os.access(path, os.X_OK)


def classify(parts):
    """Return (kind, category) for a relative path, or None if not a test."""
    if len(parts) < 3:
        return None
    key = (parts[0], parts[1])
    if key in VALID_LAYOUTS:
        return key
    return None


def invocations_for(mode, kind, category, compiler, source):
    """Return a list of (label, argv, expect_failure) or None to skip."""
    parse_cmd = [compiler, PARSE_ONLY, source]
    full_cmd = [compiler, source]

    if (kind, category) == ("pass", "semantic"):
        if mode == "syntax":
            return None
        return [
            ("parse-only", parse_cmd, False),
            ("full", full_cmd, False),
        ]

    if (kind, category) == ("fail", "semantic"):
        if mode == "syntax":
            return None
        return [
            ("parse-only", parse_cmd, False),
            ("full", full_cmd, True),
        ]

    if (kind, category) == ("fail", "syntax"):
        # In full mode, only the parse-only invocation is required.
        return [("parse-only", parse_cmd, True)]

    if (kind, category) == ("pass", "syntax"):
        # The full run is intentionally not asserted for these files.
        return [("parse-only", parse_cmd, False)]

    return None


def signal_name(signum):
    try:
        return signal.Signals(signum).name
    except ValueError:
        return "SIG{}".format(signum)


def format_timeout(value):
    seconds = float(value)
    if seconds.is_integer():
        return str(int(seconds))
    return str(seconds)


def run_invocation(cmd, timeout, cwd):
    """Run one compiler invocation.

    Returns (kind, payload):
        ("done", returncode)
        ("timeout", None)
        ("error", message)
    """
    try:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return "timeout", None
    except OSError as exc:
        return "error", str(exc)
    except Exception as exc:  # pragma: no cover - defensive
        return "error", "{}: {}".format(type(exc).__name__, exc)
    return "done", proc.returncode


def run_test(mode, kind, category, compiler, source, timeout):
    """Run all invocations for one file.

    Returns (status, detail_lines, status_suffix).
    """
    specs = invocations_for(mode, kind, category, compiler, source)
    if specs is None:
        return "SKIP", [], "  (syntax mode)"

    try:
        with tempfile.TemporaryDirectory(prefix="lsp-test-") as workdir:
            for label, cmd, expect_failure in specs:
                outcome, payload = run_invocation(cmd, timeout, workdir)

                if outcome == "timeout":
                    return (
                        "TIMEOUT",
                        [],
                        "  ({}, >{}s)".format(label, format_timeout(timeout)),
                    )
                if outcome == "error":
                    return (
                        "FAIL",
                        ["{}: could not run compiler: {}".format(label, payload)],
                        "",
                    )

                returncode = payload
                if returncode < 0:
                    return (
                        "CRASH",
                        [],
                        "  ({}, signal {})".format(label, signal_name(-returncode)),
                    )

                if expect_failure:
                    ok = returncode != 0
                    expected = "!= 0"
                else:
                    ok = returncode == 0
                    expected = "0"

                if not ok:
                    return (
                        "FAIL",
                        ["{}: expected {}, got {}".format(label, expected, returncode)],
                        "",
                    )
    except Exception as exc:  # pragma: no cover - defensive
        return (
            "FAIL",
            ["unexpected error: {}: {}".format(type(exc).__name__, exc)],
            "",
        )

    return "PASS", [], ""


def main(argv=None):
    args = build_arg_parser().parse_args(argv)

    if args.timeout <= 0:
        print("error: --timeout must be a positive number", file=sys.stderr)
        return EXIT_CONFIG_ERROR

    compiler = os.path.abspath(os.path.expanduser(args.compiler))
    if not is_executable(compiler):
        print(
            "error: compiler is missing or not executable: {}".format(args.compiler),
            file=sys.stderr,
        )
        return EXIT_CONFIG_ERROR

    tests_dir = os.path.abspath(os.path.expanduser(args.tests_dir))
    if not os.path.isdir(tests_dir):
        print(
            "error: tests directory does not exist or is not a directory: {}".format(
                args.tests_dir
            ),
            file=sys.stderr,
        )
        return EXIT_CONFIG_ERROR

    pattern = None
    if args.filter is not None:
        try:
            pattern = re.compile(args.filter)
        except re.error as exc:
            print("error: invalid --filter regex: {}".format(exc), file=sys.stderr)
            return EXIT_CONFIG_ERROR

    discovered = []
    for dirpath, dirnames, filenames in os.walk(tests_dir):
        dirnames.sort()
        for name in sorted(filenames):
            if name.endswith(".lsp"):
                discovered.append(os.path.join(dirpath, name))
    discovered.sort()

    if not discovered:
        print(
            "error: no .lsp test files found under {}".format(args.tests_dir),
            file=sys.stderr,
        )
        return EXIT_CONFIG_ERROR

    counts = {"PASS": 0, "FAIL": 0, "SKIP": 0, "TIMEOUT": 0, "CRASH": 0}
    total = 0

    for source in discovered:
        rel = os.path.relpath(source, tests_dir)
        rel_parts = rel.split(os.sep)
        rel_posix = rel.replace(os.sep, "/")

        info = classify(rel_parts)
        if info is None:
            continue

        display = os.path.join(args.tests_dir, rel)
        if pattern is not None:
            display_posix = display.replace(os.sep, "/")
            if not (
                pattern.search(rel_posix) or pattern.search(display_posix)
            ):
                continue

        kind, category = info
        total += 1

        status, details, suffix = run_test(
            args.mode, kind, category, compiler, source, args.timeout
        )
        counts[status] += 1

        # Mode-based skips stay silent unless the user asked for a filter,
        # in which case showing them makes the selection transparent.
        if status == "SKIP" and args.filter is None:
            continue

        print(status + "  " + display + suffix)
        for line in details:
            print("      " + line)
        sys.stdout.flush()

    print()
    print(
        "Total: {}   PASS: {}   FAIL: {}   SKIP: {}   TIMEOUT: {}   CRASH: {}".format(
            total,
            counts["PASS"],
            counts["FAIL"],
            counts["SKIP"],
            counts["TIMEOUT"],
            counts["CRASH"],
        )
    )
    print("Compiler: {}".format(compiler))
    print("Mode: {}".format(args.mode))

    if counts["FAIL"] or counts["TIMEOUT"] or counts["CRASH"]:
        return EXIT_TEST_FAILURE
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
