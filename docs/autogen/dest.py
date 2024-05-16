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
    * Copy ``<args.site>/docs/main/.`` to ``<args.site>/.``

    It is left to another script to commit / push the GitHUB pages repository.
"""

import argparse
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
    main_path = args.site / "docs" / "main" / "."
    root_path = args.site / "."

    # Clean the github-pages repository
    remove_except(args.site, KEEPLIST)

    # Add 'ref' to preview / archive
    if ref_path.exists():
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, ref_path)

    # Add 'main' to root
    print(f"Copying from: '{main_path}' to '{root_path}'")
    shutil.copytree(main_path, root_path, dirs_exist_ok=True)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
