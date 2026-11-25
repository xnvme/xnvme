#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Check that the parts of libxnvme depend on each other in one direction only

The core uses neither the command-line library nor the printers of the
specification, and the command-line library does not use those printers. Held,
each part can become a library of its own, which the core alone can be linked
without.

Usage: lib_parts.py <nm> <core.a> <cli.a> <pp.a>
"""

import subprocess
import sys


def symbols(nm, archive):
    """The external symbols that 'archive' defines, and those it uses"""

    out = subprocess.run(
        [nm, "-g", archive], capture_output=True, text=True, check=True
    )
    defined, used = set(), set()
    for line in out.stdout.splitlines():
        fields = line.split()
        if len(fields) < 2 or len(fields[-2]) != 1:
            continue
        kind, name = fields[-2], fields[-1]
        if kind == "U":
            used.add(name)
        elif kind.isupper():
            defined.add(name)

    return defined, used - defined


def main(args):
    if len(args) != 4:
        print(__doc__)
        return 2

    nm, *archives = args
    core, cli, pp = (symbols(nm, archive) for archive in archives)

    errors = 0
    for user, user_name, provider, provider_name in [
        (core, "core", cli, "cli"),
        (core, "core", pp, "pp"),
        (cli, "cli", pp, "pp"),
    ]:
        for name in sorted(user[1] & provider[0]):
            print(f"{user_name} uses '{name}' of {provider_name}")
            errors += 1

    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
