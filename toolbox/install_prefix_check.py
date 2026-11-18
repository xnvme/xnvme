#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Fails when 'meson install' would write a file outside the directories the build is configured for

A build for a prefix of its own, as a developer's or a CI job's, must not write into the system,
and fails to install where it may not. The directories are the prefix, and those given to meson as
absolute paths, such as the sysconfdir '/etc' of the prefix '/usr'.

    install_prefix_check.py BUILD_ROOT DIR...
"""
import json
import sys
from pathlib import Path


def main(build_root, dirs):
    installed = json.loads(
        (Path(build_root) / "meson-info" / "intro-installed.json").read_text()
    )
    outside = sorted(
        dst
        for dst in installed.values()
        if not any(Path(dst).is_relative_to(d) for d in dirs)
    )
    for dst in outside:
        print(f"installed outside of {', '.join(dirs)}: {dst}")

    return 1 if outside else 0


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__.strip().splitlines()[-1].strip())
        sys.exit(64)
    sys.exit(main(sys.argv[1], sys.argv[2:]))
