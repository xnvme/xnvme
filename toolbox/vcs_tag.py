#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Report the revision the sources came from

Run by meson as the command of the vcs_tag() target that produces 'xnvme_vcs.h',
and again by 'meson dist' to carry the revision into the source archive, which
has no git metadata. The value is 'git describe --tags --dirty=+ --always'; a
trailing '+' means the tree had uncommitted changes.

Without arguments: print the revision, taken from git when the source tree is a
checkout and from the '.vcs_tag' file a dist archive carries otherwise. Exit
non-zero when neither is available, so meson falls back to 'unknown'.

With '--dist': write the revision into '.vcs_tag' under MESON_DIST_ROOT.
"""
import os
import subprocess
import sys
from pathlib import Path

STAMP = ".vcs_tag"


def from_git(root):
    try:
        out = subprocess.run(
            ["git", "describe", "--tags", "--dirty=+", "--always"],
            cwd=root,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None
    return out or None


def main():
    root = Path(os.environ.get("MESON_SOURCE_ROOT", os.getcwd()))

    if "--dist" in sys.argv[1:]:
        tag = from_git(root)
        if not tag:
            print("vcs_tag: no git revision to record", file=sys.stderr)
            return 1
        (Path(os.environ["MESON_DIST_ROOT"]) / STAMP).write_text(tag + "\n")
        return 0

    tag = from_git(root)
    if not tag:
        stamp = root / STAMP
        tag = stamp.read_text().strip() if stamp.is_file() else None
    if not tag:
        return 1
    print(tag)
    return 0


if __name__ == "__main__":
    sys.exit(main())
