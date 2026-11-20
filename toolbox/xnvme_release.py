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

With --archive PATH it checks the source archive at PATH against the version of
the tree, and prints what is wrong with it, one problem per line; see
archive_problems(). --subprojects names the subprojects the archive must carry,
and --release says that the archive is that of a release.
"""

import argparse
import posixpath
import re
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path, PurePosixPath

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


def version_of(root: Path) -> str:
    """The version that meson.build under 'root' carries"""

    path, pattern = VERSIONED[0]
    found = re.search(pattern, (root / path).read_text(), re.M)
    if not found:
        raise ValueError(f"{path}: no match of {pattern!r}")

    return found.group(1)


def archive_problems(path: Path, version: str, subprojects=(), release=False):
    """What is wrong with the source archive at 'path' for a release of 'version'

    Every member lies under the directory 'xnvme-<version>/', which holds no git
    metadata and no link to outside it or to a file it does not hold. The files
    carrying the version carry 'version', as check() reads them, CHANGELOG.rst
    has its section, and '.vcs_tag' records the revision, as a build of the
    archive has no git to ask; when 'release', that revision is the tag
    'v<version>'. Each of 'subprojects' is a directory of files.
    """

    top = PurePosixPath(f"xnvme-{version}")
    problems = []
    with tarfile.open(path, "r:gz") as tar:
        members = tar.getmembers()
        names = {PurePosixPath(member.name) for member in members}
        files = {PurePosixPath(m.name) for m in members if m.isfile()}

        for member in members:
            name = PurePosixPath(member.name)
            if (
                name.is_absolute()
                or ".." in name.parts
                or name.parts[:1] != (top.name,)
            ):
                problems.append(f"{member.name}: not under {top}/")
                continue
            if ".git" in name.parts:
                problems.append(f"{member.name}: git metadata")
            if member.issym() or member.islnk():
                base = name.parent if member.issym() else PurePosixPath()
                target = PurePosixPath(posixpath.normpath(base / member.linkname))
                if member.linkname.startswith("/") or target.parts[:1] != (top.name,):
                    problems.append(
                        f"{member.name}: a link to {member.linkname}, outside {top}/"
                    )
                elif target not in names:
                    problems.append(
                        f"{member.name}: a link to {member.linkname}, which the archive does not hold"
                    )

        if top / ".vcs_tag" not in files:
            problems.append(
                f"{top}/.vcs_tag: missing, so a build of the archive reports no revision"
            )
        elif release:
            stamp = tar.extractfile(str(top / ".vcs_tag")).read()
            revision = stamp.decode(errors="replace").strip()
            if revision != f"v{version}":
                problems.append(
                    f"{top}/.vcs_tag: {revision}, not the tag v{version} of the release"
                )
        for subproject in subprojects:
            directory = top / "subprojects" / subproject
            if not any(directory in name.parents for name in files):
                problems.append(f"{directory}/: missing or without files")

        needed = sorted({path for path, _ in VERSIONED} | {"CHANGELOG.rst"})
        missing = [p for p in needed if top / p not in files]
        problems += [f"{top}/{p}: missing" for p in missing]
        if not missing:
            with tempfile.TemporaryDirectory() as tmp:
                for p in needed:
                    dest = Path(tmp) / p
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    dest.write_bytes(tar.extractfile(str(top / p)).read())
                try:
                    check(f"ver: bump to v{version}", Path(tmp))
                except ValueError as exc:
                    problems.append(str(exc))

    return problems


def main():
    prsr = argparse.ArgumentParser(description="Check a version bump, print its notes")
    prsr.add_argument("subject", nargs="?", help="Subject of the commit")
    mode = prsr.add_mutually_exclusive_group()
    mode.add_argument("--bump", metavar="X.Y.Z", help="Set the version everywhere")
    mode.add_argument(
        "--contributors", action="store_true", help="Authors CONTRIBUTORS.md misses"
    )
    mode.add_argument(
        "--archive", type=Path, help="Check a source archive against the tree"
    )
    prsr.add_argument(
        "--subprojects",
        default="",
        help="Comma-separated subprojects the archive must carry",
    )
    prsr.add_argument(
        "--release",
        action="store_true",
        help="With --archive: the archive is that of a release",
    )
    prsr.add_argument("--root", type=Path, default=Path.cwd())
    args = prsr.parse_args()
    if args.subject and (args.bump or args.contributors or args.archive):
        prsr.error("give a commit subject, --bump, --contributors or --archive")

    try:
        if args.bump:
            bump(args.bump, args.root)
        elif args.contributors:
            for name in missing_contributors(args.root):
                print(name)
        elif args.archive:
            subprojects = [s for s in args.subprojects.split(",") if s]
            problems = archive_problems(
                args.archive, version_of(args.root), subprojects, args.release
            )
            for problem in problems:
                print(problem)
            if problems:
                return 1
        elif args.subject:
            _, notes = check(args.subject, args.root)
            print(notes)
        else:
            prsr.error("give a commit subject, --bump, --contributors or --archive")
    except ValueError as exc:
        print(f"release: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
