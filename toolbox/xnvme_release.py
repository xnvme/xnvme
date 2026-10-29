#!/usr/bin/env python3

# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Whether a commit releases a version, and the notes to release it with

Given the subject of the commit, 'ver: bump to vX.Y.Z', this checks that every
file carrying the version carries X.Y.Z and that CHANGELOG.rst has a section
for vX.Y.Z, and writes that section to stdout as the release notes. It exits
non-zero, saying why, when any of that is not so.
"""

import argparse
import re
import sys
from pathlib import Path

SUBJECT = re.compile(r"^ver: bump to v(\d+\.\d+\.\d+)$")

VERSIONED = [
    ("meson.build", r"^\s*version: '([^']+)',"),
    ("python/bindings/setup.py", r'^\s*version="([^"]+)",'),
    ("rust/xnvme-sys/Cargo.toml", r'^version = "([^"]+)"'),
    ("rust/xnvme-sys/Cargo.toml", r'^xnvme = "([^"]+)"'),
    ("cijoe/pyproject.toml", r'^version = "([^"]+)"'),
]


def notes_for(changelog: str, version: str) -> str:
    """The text of the 'vX.Y.Z' section, or '' when there is none"""

    section = re.search(
        rf"^v{re.escape(version)}\n-+\n(.*?)(?=^\S[^\n]*\n-+\n|\Z)",
        changelog,
        re.M | re.S,
    )
    return section.group(1).strip() if section else ""


def check(subject: str, root: Path):
    """The version and its notes; raises ValueError saying what is wrong"""

    bump = SUBJECT.match(subject.strip())
    if not bump:
        raise ValueError(f"not a version bump: {subject!r}")
    version = bump.group(1)

    wrong = []
    for path, pattern in VERSIONED:
        found = re.search(pattern, (root / path).read_text(), re.M)
        if not found or found.group(1) != version:
            wrong.append(f"{path} has {found.group(1) if found else 'none'}")
    if wrong:
        raise ValueError(f"the commit bumps to {version}, but " + ", ".join(wrong))

    notes = notes_for((root / "CHANGELOG.rst").read_text(), version)
    if not notes:
        raise ValueError(f"CHANGELOG.rst has no 'v{version}' section")

    return version, notes


def main():
    prsr = argparse.ArgumentParser(description="Check a version bump, print its notes")
    prsr.add_argument("subject", help="Subject of the commit")
    prsr.add_argument("--root", type=Path, default=Path.cwd())
    args = prsr.parse_args()

    try:
        _, notes = check(args.subject, args.root)
    except ValueError as exc:
        print(f"release: {exc}", file=sys.stderr)
        return 1

    print(notes)

    return 0


if __name__ == "__main__":
    sys.exit(main())
