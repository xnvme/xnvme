#!/usr/bin/env python3
"""
    This script organizes the website and documentation for xnvme.io

    When running ``make`` inside of ``<xnvme_repository>/docs/autogen/`` then an
    instance of the website / documentation is emitted at::

        <xnvme_repository>/docs/autogen/builddir/html

    This path is given to the script via ``<args.docs>`.

    The documention/website lives in a repository dedicated to GitHUB pages, a path
    to this repository is given to the script via ``<args.site>``.

    The intent of the organization is two-fold:

    * Provide a preview of website/documentation of the current state of the 'next'
      branch, as well as other in-development branches

    * Keep a bit of history on the project documentation available online, e.g. for
      those not on the latest release.

    Supported refs. are on the form:

        refs/heads/<branch_name>
        refs/tags/<tag_name>

    The organization is as follows, for all refs, then the folder ``<args.docs>`` is
    renamed to ``<args.ref>`` and is placed in ``<args.site>/docs/<args.ref>``, here are
    a couple of examples::

        <args.site>/docs/main
        <args.site>/docs/next
        <args.site>/docs/v1.2.3

    This is the archive/history part of the organization. The latest version of the
    website and documentation lives in the root of ``<args.site>``:

        <args.site>/.

    In addition to the things emitted by sphinx-doc, then there are a couple of files
    specific to GitHub pages, things like ``.nojekyll``, a README.md and stuff like
    that, these files and the folder ``docs`` are recorded in a ``KEEPLIST``.

    This organization is done by:

    * Removing everything in ``<args.site>`` that is not in KEEPLIST
    * Copy ``<args.docs>`` to ``<args.site>/docs/<args.ref>``
    * Copy ``<args.site>/docs/<args.current>/.`` to ``<args.site>/.``
    * Emit a PyData versions.json docs in ``<args.site>``
      - Note that the script organizes instances of the documentation that are not
        exposed in the PyData versions.json
      - The instances of the documentation that are exposed in the PyData versions.json
        are named as a version-tag (vMAJOR.MINOR.PATCH), or;
        ``current`` and ``next``

    It is left to another script to commit / push the GitHUB pages repository.
"""

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

KEEPLIST = [
    "CNAME",
    "docs",
    "favicon.ico",
    ".git",
    ".nojekyll",
    "README.md",
]

REGEX_VERSION_TAG = r"v(\d+)\.(\d+)\.(\d+)"


def parse_args():
    """Parse command-line arguments for docs destination script"""

    # Parse the Command-Line
    prsr = argparse.ArgumentParser(
        description="xNVMe documentation organizer",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--docs",
        help="Path to SphinxDoc generated HTML",
        default=Path.cwd(),
        type=Path,
    )
    prsr.add_argument(
        "--site",
        help="Path to xNVMe.io GitHUB Repository",
        type=Path,
        default=Path.cwd(),
    )
    prsr.add_argument("--ref", help="xNVMe repository reference")
    prsr.add_argument(
        "--current",
        help="The instance from docs/ to place in site-root",
        type=str,
        default="main",
    )
    args = prsr.parse_args()

    return args


def remove_except(path, keep):
    """
    Remove all files and folders in the given 'path' except for those named in 'keep'
    """

    for item in path.iterdir():
        if item.name not in keep:
            if item.is_file() or item.is_symlink():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)


def gen_versions(path: Path, current: str):
    """Returns a dict compatible with the Pydata versions-format"""

    def version_key(name):
        """
        Here is a brief example on the ordering provided by this function:

        current
        feature-a
        feature-b
        next
        visual
        xperiment
        v1.2.3
        v0.1.2
        v0.0.1

        In other words; branch names before tags, tags in descending order, and tags are
        assumed to be on the form: vMAJOR.MINOR.PATCH.
        """

        match = re.match(REGEX_VERSION_TAG, name)
        if match:
            return (1, -int(match.group(1)), -int(match.group(2)), -int(match.group(3)))
        return (0, name)

    branch_names = ["current", "next"]  # Only include these branch-names
    versions = [
        {
            "name": f"{current}",
            "version": f"{current}",
            "url": "https://xnvme.io/",
            "preferred": True,
        },
    ]

    for name in sorted((d.name for d in path.iterdir()), key=version_key):
        if not (re.match(REGEX_VERSION_TAG, name) or name in branch_names):
            continue

        versions.append(
            {
                "name": f"{name}",
                "version": f"{name}",
                "url": f"/docs/{name}",
            }
        )

    return versions


def dict_to_path(data: dict, path: Path):
    """Write the given dict to file path"""

    with path.open(mode="w", encoding="utf-8") as file:
        json.dump(data, file, indent=4, ensure_ascii=False)


def main(args):
    """
    Add the given 'args.docs' to 'args.site/docs/<ref>'
    For tags also add to 'args.site/docs/latest'
    In both cases existing docs are removed
    """

    # Verify the given ref matches assumptions
    is_tag = "tags" in args.ref
    is_branch = "heads" in args.ref
    if not (is_tag or is_branch):
        print(f"Failed: ref('{args.ref}') is neither tag nor branch")
        return 1
    if "/" not in args.ref or len(args.ref.split("/")) < 3:
        print(f"Failed: ref('{args.ref}') has unexpected format")
        return 1

    ref = "-".join(args.ref.split("/")[2:])

    # Setup paths
    args.docs = args.docs.resolve()
    args.site = args.site.resolve()

    ref_path = args.site / "docs" / ref
    cur_path = args.site / "docs" / args.current / "."
    root_path = args.site / "."

    # Clean the github-pages repository
    remove_except(args.site, KEEPLIST)

    # Add 'ref' to preview / archive
    if ref_path.exists():
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, ref_path)

    # Add the "current" site
    print(f"Copying from: '{cur_path}' to '{root_path}'")
    shutil.copytree(cur_path, root_path, dirs_exist_ok=True)

    # Dump the versions
    data = gen_versions(args.site / "docs", args.current)
    dict_to_path(data, args.site / "versions.json")

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
