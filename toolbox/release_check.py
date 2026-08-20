#!/usr/bin/env python3

# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

"""
    Check whether the tree is ready to be tagged as a release

    Answers "did I miss a step" for the parts of the release checklist that a
    machine can settle, so the answer is an exit code rather than a claim. What
    it does not check is listed by --help and in the release checklist; man
    pages and Bash completions in particular carry no version, so their
    staleness cannot be established without regenerating them.

    When running from shell, returns 0 when ready, 1 otherwise
"""
import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from xnvme_ver import FILES, VER_PATTERN, xnvme_ver  # noqa: E402


def git(root, *args):
    """Returns the stripped stdout of a git command, or '' when it fails"""

    result = subprocess.run(
        ["git", "-C", root, *args], capture_output=True, text=True, check=False
    )

    return result.stdout.strip() if result.returncode == 0 else ""


def check_version_consistent(root, version):
    """Every file embedding the version agrees with meson.build"""

    problems = []

    for filename, pattern, _ in FILES:
        path = os.path.join(root, filename)
        try:
            with open(path) as ofile:
                content = ofile.read()
        except FileNotFoundError:
            problems.append(f"{filename}: missing")
            continue

        found = {match.group(1) for match in re.finditer(pattern, content)}
        if not found:
            problems.append(f"{filename}: no version found, the pattern went stale")
        elif found != {version}:
            problems.append(f"{filename}: has {sorted(found)}, expected {version}")

    return problems


def check_tag_absent(root, version):
    """The tag is not already taken, so this is not a re-release by accident"""

    if git(root, "tag", "--list", f"v{version}"):
        return [f"tag v{version} already exists"]

    return []


def check_changelog(root, version):
    """CHANGELOG.rst carries a section for the version being released"""

    path = os.path.join(root, "CHANGELOG.rst")
    try:
        with open(path) as ofile:
            content = ofile.read()
    except FileNotFoundError:
        return ["CHANGELOG.rst: missing"]

    if not re.search(rf"(?m)^v{re.escape(version)}\s*$", content):
        return [f"CHANGELOG.rst: no section for v{version}"]

    return []


def contributors_since(root, previous):
    """Authors with commits since 'previous', as 'Name <email>'"""

    log = git(root, "log", f"{previous}..HEAD", "--pretty=format:%an <%aE>")

    return {line for line in log.splitlines() if line}


def check_contributors(root, previous):
    """CONTRIBUTORS.md names everyone who has committed since the last tag"""

    path = os.path.join(root, "CONTRIBUTORS.md")
    try:
        with open(path) as ofile:
            content = ofile.read()
    except FileNotFoundError:
        return ["CONTRIBUTORS.md: missing"], set()

    missing = set()
    for author in contributors_since(root, previous):
        name = author.split("<")[0].strip()
        email = author.split("<")[1].rstrip(">") if "<" in author else ""
        if name not in content and email not in content:
            missing.add(author)

    if missing:
        return [
            f"CONTRIBUTORS.md: {len(missing)} author(s) since {previous} not listed"
        ], missing

    return [], set()


def check_tree_clean(root):
    """Nothing uncommitted, so what is checked is what would be tagged"""

    if git(root, "status", "--porcelain"):
        return ["working tree is dirty"]

    return []


def parse_args():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description="Check whether the tree is ready to be tagged as a release",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog=(
            "Not checked, and left to the releaser: that the man pages and Bash"
            " completions were regenerated, since neither carries a version; that"
            " the CHANGELOG section describes the release rather than merely"
            " existing; and everything after the tag, which is deliberately manual"
            " because it cannot be undone."
        ),
    )
    prsr.add_argument("--path", help="Path to 'meson.build'", default="meson.build")
    prsr.add_argument(
        "--previous",
        help="Tag to compare against; defaults to the most recent vX.Y.Z",
    )
    prsr.add_argument(
        "--contributors",
        help="Print the authors missing from CONTRIBUTORS.md and exit",
        action="store_true",
    )

    return prsr.parse_args()


def main(args):
    """Entry point"""

    path = os.path.abspath(os.path.expanduser(os.path.expandvars(args.path)))
    root = os.path.dirname(path)

    try:
        version = xnvme_ver(path)
    except FileNotFoundError:
        print(f"error: no such file: {path}", file=sys.stderr)
        return 1

    if not re.fullmatch(VER_PATTERN, version):
        print(f"error: '{version}' is not on the form 'x.y.z'", file=sys.stderr)
        return 1

    previous = args.previous
    if not previous:
        tags = git(root, "tag", "--list", "v[0-9]*", "--sort=-v:refname").splitlines()
        previous = tags[0] if tags else ""

    contributor_problems, missing = ([], set())
    if previous:
        contributor_problems, missing = check_contributors(root, previous)

    if args.contributors:
        for author in sorted(missing):
            print(author)
        return 0

    problems = (
        check_version_consistent(root, version)
        + check_tag_absent(root, version)
        + check_changelog(root, version)
        + contributor_problems
        + check_tree_clean(root)
    )

    print(f"xNVMe version: {version}")
    print(f"previous tag: {previous or 'none found'}")

    if not problems:
        print("ready: every checked item is in order")
        return 0

    print(f"not ready: {len(problems)} item(s) need attention")
    for problem in problems:
        print(f"  - {problem}")
    if missing:
        print("    run with --contributors to list them")

    return 1


if __name__ == "__main__":
    sys.exit(main(parse_args()))
