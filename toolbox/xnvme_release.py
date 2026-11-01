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

With --bump X.Y.Z it instead writes X.Y.Z into every file carrying the
version, and with --contributors it prints the authors since the previous tag
that CONTRIBUTORS.md does not name.
"""

import argparse
import re
import subprocess
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


def bump(version: str, root: Path):
    """Write 'version' into every file carrying it, or into none of them

    Each file's first match is replaced, the one check() reads; a file with no
    match fails the bump rather than being left at the old version.
    """

    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError(f"not a version: {version!r}")

    texts = {}
    for path, pattern in VERSIONED:
        text = texts.get(path, (root / path).read_text())
        found = re.search(pattern, text, re.M)
        if not found:
            raise ValueError(f"{path}: no match of {pattern!r}")
        start, end = found.span(1)
        texts[path] = text[:start] + version + text[end:]

    for path, text in texts.items():
        (root / path).write_text(text)


def missing_contributors(root: Path):
    """Authors since the previous tag whom CONTRIBUTORS.md does not name"""

    def git(*args):
        try:
            return subprocess.run(
                ["git", "-C", str(root), *args],
                capture_output=True,
                text=True,
                check=True,
            ).stdout
        except subprocess.CalledProcessError as exc:
            raise ValueError(exc.stderr.strip()) from exc

    tag = git("describe", "--tags", "--abbrev=0", "--match", "v[0-9]*").strip()
    authors = set(git("log", "--format=%aN", f"{tag}..HEAD").splitlines())
    try:
        contributors = (root / "CONTRIBUTORS.md").read_text()
    except FileNotFoundError as exc:
        raise ValueError(str(exc)) from exc
    named = set(re.findall(r"^\* (.+?) <", contributors, re.M))

    return sorted(authors - named)


def main():
    prsr = argparse.ArgumentParser(description="Check a version bump, print its notes")
    prsr.add_argument("subject", nargs="?", help="Subject of the commit")
    mode = prsr.add_mutually_exclusive_group()
    mode.add_argument("--bump", metavar="X.Y.Z", help="Set the version everywhere")
    mode.add_argument(
        "--contributors", action="store_true", help="Authors CONTRIBUTORS.md misses"
    )
    prsr.add_argument("--root", type=Path, default=Path.cwd())
    args = prsr.parse_args()
    if args.subject and (args.bump or args.contributors):
        prsr.error("give a commit subject, --bump or --contributors, not both")

    try:
        if args.bump:
            bump(args.bump, args.root)
        elif args.contributors:
            for name in missing_contributors(args.root):
                print(name)
        elif args.subject:
            _, notes = check(args.subject, args.root)
            print(notes)
        else:
            prsr.error("give a commit subject, --bump or --contributors")
    except ValueError as exc:
        print(f"release: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
