#!/usr/bin/env python3
"""
    clang-format wrapper for pre-commit integration with external style-file

    In clang-format-14 one can specify the path to clang-format-definitions, however,
    clang-format-14 is at the time of writing this not released and therefore a bit
    cumbersome to distribute. This script wraps the invocation of clang-format in a
    manner such that it transforms the given file into -style arguments as well as
    invoking clang-format in a manner such that it reports issues, fixes them, and
    instructs the comitter how to see inspect the auto-charges performed.

    When running from shell, returns 0 on success, some other value otherwise
"""
import argparse
import os
import subprocess
import sys


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def parse_args():
    """Parse command-line arguments"""

    parser = argparse.ArgumentParser(
        description="Wrap clang-format",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--style-file",
        help="Path to clang-format style definitions",
        required=True,
    )
    parser.add_argument("clang_format_args", nargs=argparse.REMAINDER)

    args = parser.parse_args()
    args.style_file = expand_path(args.style_file)

    return args


def clang_format(args):
    """Invoke clang-format"""

    # Form arguments to clang-format parameter "-style"
    with open(args.style_file) as f:
        style = ",".join(
            [line.strip() for line in f if ":" in line and "#" not in line]
        )
    cmd_dry = "clang-format -style='{%s}' -i --dry-run -Werror %s" % (
        style,
        " ".join(args.clang_format_args),
    )
    cmd_mod = "clang-format -style='{%s}' -i %s" % (
        style,
        " ".join(args.clang_format_args),
    )

    res = subprocess.run(cmd_dry, shell=True)
    if res.returncode:
        subprocess.run(cmd_mod, shell=True)
        print("Reformatted ...")

    return res.returncode


def main(args):
    """Entry point"""

    try:
        return clang_format(args)
    except FileNotFoundError:
        return 1


if __name__ == "__main__":
    sys.exit(main(parse_args()))
