#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Reject commit messages containing non-ASCII bytes.

Pre-commit invokes this at the commit-msg stage with the path to the
commit message file as the first argument.
"""
import sys


def main():
    if len(sys.argv) < 2:
        print("usage: pcm_ascii_only.py <commit-msg-file>", file=sys.stderr)
        return 2

    with open(sys.argv[1], "rb") as fh:
        data = fh.read()

    offenders = []
    for lineno, line in enumerate(data.splitlines(), 1):
        for col, byte in enumerate(line, 1):
            if byte > 127:
                offenders.append((lineno, col, byte))

    if not offenders:
        return 0

    for lineno, col, byte in offenders:
        print(
            f"non-ASCII byte 0x{byte:02x} at line {lineno}, column {col}",
            file=sys.stderr,
        )
    print(
        "ERROR: commit message contains non-ASCII bytes; rewrite without "
        "em dashes, smart quotes, or other Unicode glyphs.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
