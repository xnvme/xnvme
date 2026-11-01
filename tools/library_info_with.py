#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Check that 'xnvme library-info' names every with-<name> build option, and
nothing that is not one, so its 'with' section tells what the flags switched
"""

import re
import subprocess
import sys


def main(xnvme, meson_options):
    with open(meson_options) as ofd:
        options = set(re.findall(r"^option\('with-([^']+)'", ofd.read(), re.M))

    info = subprocess.run(
        [xnvme, "library-info"], capture_output=True, text=True, check=True
    ).stdout
    section = info.split("\n  with:\n", 1)[1].split("\n\n", 1)[0]
    parts = set(
        re.findall(
            r"^  - \{name: '([^']+)', macro: 'XNVME_[A-Z_]+_ENABLED', built: (?:yes|no), ",
            section,
            re.M,
        )
    )

    missing = sorted(options - parts)
    unknown = sorted(parts - options)
    if missing or unknown:
        print(
            f"options without an entry: {missing}; entries without an option: {unknown}"
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
