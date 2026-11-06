#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Check that the printers of the API are declared in libxnvme_pp.h, and in no
other libxnvme_*.h, so a binding leaves them all out by leaving out one header

The command-line library keeps its own printers. A static inline printer has
no symbol for a binding to call, and stays with what it prints.
"""

import re
import sys
from pathlib import Path

# A declaration at file scope: the return type, on the line or the one above,
# then the name of a printer
PRINTER = re.compile(
    r"^(?P<head>[A-Za-z_][\w \t*]*?)?\**(?P<name>xnvme_\w+_(?:fpr|pr|pp|fpp|str|yaml))\("
)
EXEMPT = ["libxnvme_pp.h", "libxnvme_cli.h"]


def printers(text):
    """The names of the printers declared in 'text', other than static inline ones"""

    lines = text.splitlines()
    for i, line in enumerate(lines):
        match = PRINTER.match(line)
        if not match:
            continue
        head = match["head"] or (lines[i - 1] if i else "")
        if not head.startswith("static"):
            yield match["name"]


def main(include_dir):
    offenders = [
        f"{path.name}: {name}"
        for path in sorted(Path(include_dir).glob("libxnvme_*.h"))
        if path.name not in EXEMPT
        for name in printers(path.read_text())
    ]
    if offenders:
        print(f"printers outside libxnvme_pp.h: {offenders}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
