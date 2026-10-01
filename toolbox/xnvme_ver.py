#!/usr/bin/env python3

# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

"""
    Extract and update the xNVMe version

    Without arguments, the version is read from 'meson.build' and printed. With
    '--set' or '--bump', it is written to every file embedding it; see FILES.

    When running from shell, returns 0 on success, some other value otherwise
"""
import argparse
import os
import re
import sys

VER_PATTERN = r"\d+\.\d+\.\d+"

# The files embedding the version-number, along with the pattern matching it.
#
# Each entry is (path, pattern, count) where 'pattern' has a single group
# capturing the version-number and 'count' is the amount of matches expected in
# the file. Mismatching counts are an error; that is how a file changing shape
# without this table changing with it is caught.
FILES = [
    ("meson.build", r"(?m)^  version: '(" + VER_PATTERN + r")',$", 1),
    ("python/bindings/setup.py", r'(?m)^    version="(' + VER_PATTERN + r')",$', 1),
    ("rust/xnvme-sys/Cargo.toml", r'(?m)^version = "(' + VER_PATTERN + r')"$', 1),
    ("rust/xnvme-sys/Cargo.toml", r'(?m)^xnvme = "(' + VER_PATTERN + r')"$', 1),
    ("docs/tooling/doxy.cfg", r"(?m)^PROJECT_NUMBER\s+= (" + VER_PATTERN + r")$", 1),
    ("cijoe/pyproject.toml", r'(?m)^version = "(' + VER_PATTERN + r')"$', 1),
]


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def xnvme_ver(path=None):
    """
    Retrieve the version from project CMakeLists.txt

    @returns "x.y.z", {"major": x, "minor": y, "patch": z}
    """

    if path is None:
        path = os.sep.join(["..", "..", "meson.build"])

    with open(path) as cmake:
        for line in cmake.readlines():
            if "version:" not in line:
                continue

            _, vtxt = line.split("version:", 1)

            return vtxt.replace(",", "").replace("'", "").strip()

    return ""


def bumped(version, part):
    """Returns 'version' with the given 'part' incremented"""

    major, minor, patch = (int(field) for field in version.split("."))

    if part == "major":
        return f"{major + 1}.0.0"
    if part == "minor":
        return f"{major}.{minor + 1}.0"

    return f"{major}.{minor}.{patch + 1}"


def update(root, version, dry_run=False):
    """
    Replace the version-number in every file in FILES

    @returns list of (path, before, after) for the replacements made
    """

    changes = []
    rewritten = []

    # Check every file before writing any of them; a partially bumped tree is
    # worse than an unbumped one, since nothing says which half was done
    for filename, pattern, count in FILES:
        path = os.path.join(root, filename)

        with open(path) as ofile:
            content = ofile.read()

        matches = list(re.finditer(pattern, content))
        if len(matches) != count:
            raise RuntimeError(
                f"{filename}: expected {count} match(es) of {pattern!r},"
                f" got {len(matches)}"
            )

        for match in matches:
            changes.append((filename, match.group(1), version))

        rewritten.append(
            (
                path,
                re.sub(
                    pattern,
                    lambda match: match.group(0).replace(match.group(1), version),
                    content,
                ),
            )
        )

    if dry_run:
        return changes

    for path, content in rewritten:
        with open(path, "w") as ofile:
            ofile.write(content)

    return changes


def parse_args():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description="Extract or update the xNVMe version",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument("--path", help="Path to 'meson.build'", default="meson.build")
    prsr.add_argument("--set", help="Set the version to 'x.y.z'")
    prsr.add_argument(
        "--bump",
        help="Increment the given part of the version",
        choices=["major", "minor", "patch"],
    )
    prsr.add_argument(
        "--dry-run",
        help="Print what would change without writing it",
        action="store_true",
    )
    args = prsr.parse_args()
    args.path = expand_path(args.path)

    if args.set and args.bump:
        prsr.error("give either --set or --bump, not both")
    if args.set and not re.fullmatch(VER_PATTERN, args.set):
        prsr.error(f"--set: '{args.set}' is not on the form 'x.y.z'")

    return args


def main(args):
    """Entry point"""

    try:
        current = xnvme_ver(args.path)
    except FileNotFoundError:
        return 1

    if not (args.set or args.bump):
        print(current)
        return 0

    version = args.set if args.set else bumped(current, args.bump)
    root = os.path.dirname(args.path)

    try:
        changes = update(root, version, args.dry_run)
    except (FileNotFoundError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    for filename, before, after in changes:
        print(f"{filename}: {before} -> {after}")

    print(f"xNVMe version: {current} -> {version}")

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
