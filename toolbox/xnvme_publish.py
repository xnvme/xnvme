#!/usr/bin/env python3

# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Upload a release of xNVMe to PyPI and crates.io from the assets of its GitHub release

'fetch X.Y.Z' downloads the Python sdist and the source archive of the GitHub
release vX.Y.Z with gh, checks each against its .sha256sum, and writes the
sdist by the name PyPI requires, <dir>/pypi/xnvme-X.Y.Z.tar.gz, and the source
archive unpacked under <dir>/src. 'pypi X.Y.Z' fetches, then uploads the sdist
with twine; 'crate X.Y.Z' fetches, then uploads xnvme-sys from the source
archive with cargo, unless crates.io has that version already.

The 'pypi' and 'crates-io' jobs of the 'publish-release' workflow run 'fetch'
and 'crate'. When they fail, a maintainer runs 'make publish VERSION=X.Y.Z',
logged in to GitHub with gh, to PyPI with twine and to crates.io with cargo.
"""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

REPO = "xnvme/xnvme"

# The sparse index of crates.io, a line of JSON for each version of xnvme-sys
INDEX = "https://index.crates.io/xn/vm/xnvme-sys"


def assets(version):
    """The names of the sdist and the source archive of the release 'version'"""

    return f"xnvme-py-sdist-v{version}.tar.gz", f"xnvme-{version}.tar.gz"


def download(version, directory):
    """Download the assets() of the release and their .sha256sum to 'directory'"""

    patterns = []
    for name in assets(version):
        patterns += ["--pattern", name, "--pattern", f"{name}.sha256sum"]
    subprocess.run(
        ["gh", "release", "download", f"v{version}", "--repo", REPO]
        + ["--dir", str(directory)]
        + patterns,
        check=True,
    )


def verify(path):
    """Raise ValueError unless 'path' has the digest of its .sha256sum"""

    expected = Path(f"{path}.sha256sum").read_text().split()[0]
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != expected:
        raise ValueError(f"{path.name} has the sha256 {digest}, not {expected}")


def fetch(version, directory, download=download):
    """The sdist, by the name PyPI requires, and the manifest of xnvme-sys in the
    unpacked source archive, of the release 'version', fetched to 'directory'"""

    got = directory / "assets"
    got.mkdir(parents=True)
    download(version, got)
    sdist, source = assets(version)
    for name in (sdist, source):
        verify(got / name)

    pypi = directory / "pypi" / f"xnvme-{version}.tar.gz"
    pypi.parent.mkdir()
    shutil.copy(got / sdist, pypi)
    with tarfile.open(got / source) as tar:
        tar.extractall(directory / "src", filter="data")

    return pypi, directory / "src" / f"xnvme-{version}" / "rust/xnvme-sys/Cargo.toml"


def on_crates_io(version, index=None):
    """Whether crates.io has xnvme-sys 'version', by the text of its 'index'"""

    if index is None:
        with urllib.request.urlopen(INDEX) as response:
            index = response.read().decode()

    return any(
        json.loads(line).get("vers") == version
        for line in index.splitlines()
        if line.strip()
    )


def upload(command, version, sdist, manifest, index=None):
    """The command line that uploads the release 'version' for 'command', from
    the fetched 'sdist' and 'manifest', or None when there is none to run"""

    if command == "pypi":
        return ["twine", "upload", "--skip-existing", str(sdist)]
    if command == "crate" and not on_crates_io(version, index):
        # --no-verify: the build script of xnvme-sys needs an installed xNVMe of
        # this version; the 'ffi-rust' job of the release's run built it so
        return ["cargo", "publish", "--no-verify", "--manifest-path", str(manifest)]

    return None


def main():
    prsr = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    prsr.add_argument("command", choices=["fetch", "pypi", "crate"])
    prsr.add_argument("version", help="The version of the release, X.Y.Z")
    prsr.add_argument(
        "--dir",
        type=Path,
        help="Where to fetch the release to; a new temporary directory by default",
    )
    args = prsr.parse_args()
    if not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
        prsr.error(f"not a version: {args.version!r}")

    # Outside the checkout by default, as cargo fails on files that the git
    # repository around them has not committed
    directory = args.dir or Path(tempfile.mkdtemp(prefix="xnvme-publish-"))
    try:
        sdist, manifest = fetch(args.version, directory)
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        print(f"publish: {exc}", file=sys.stderr)
        return 1
    print(f"fetched v{args.version} to {directory}")

    run = upload(args.command, args.version, sdist, manifest)
    if run is None:
        if args.command == "crate":
            print(f"crates.io has xnvme-sys {args.version} already")
        return 0

    return subprocess.run(run).returncode


if __name__ == "__main__":
    sys.exit(main())
