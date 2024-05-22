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

    * Provide a preview of website/documentation of the 'next' branch / release, other
      in-development branches are not available via version-switcher, however,
      they can be pointed to manually e.g. https://xnvme.io/en/feature-a

    * Keep a bit of history on the project documentation available online, e.g. for
      those not on the latest release.

    Supported refs. are on the form:

        refs/heads/<branch_name>
        refs/tags/<tag_name>

    The organization is as follows, for all refs, then the folder ``<args.docs>`` is
    renamed to ``<args.ref>`` and is placed in ``<args.site>/en/<args.ref>``, here are
    a couple of examples::

        <args.site>/en/main
        <args.site>/en/docs
        <args.site>/en/next
        <args.site>/en/v1.2.3

    Handling of "docs" and version-tags (vX.Y.Z) is special, these are copied to
    the ``<args.site>``:

        <args.site>/.

    In addition to the things emitted by sphinx-doc, then there are a couple of files
    specific to GitHub pages, things like ``.nojekyll``, a README.md and stuff like
    that, these files and the folder ``docs`` are recorded in a ``KEEPLIST``.

    This organization is done by:

    * Removing everything in ``<args.site>`` that is not in KEEPLIST
    * Copy ``<args.docs>`` to ``<args.site>/en/<args.ref>``

    * Emit a PyData versions.json docs in ``<args.site>``
      - Note that the script organizes instances of the documentation that are not
        exposed in the PyData versions.json
      - The instances of the documentation that are exposed in the PyData versions.json
        are named as a version-tag (vMAJOR.MINOR.PATCH), or ``next``
      - The preferred version is set to the latest version
      - Updates to 'docs' will update the latest / preferred version

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
    "en",
    "favicon.ico",
    ".git",
    "_history",
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
        help="Path to sphinx-doc instance to add to website",
        default=Path.cwd(),
        type=Path,
    )
    prsr.add_argument(
        "--site",
        help="Path to GitHub Pages Repository for the website",
        type=Path,
        default=Path.cwd(),
    )
    prsr.add_argument(
        "--ref",
        help="xNVMe repository reference (e.g. refs/heads/docs or refs/tags/v1.2.3)",
    )
    prsr.add_argument(
        "--url", help="Base URL for versions.json", type=str, default="https://xnvme.io"
    )
    args = prsr.parse_args()
    args.docs = args.docs.resolve()
    args.site = args.site.resolve()

    return args


def remove_except(args, keep):
    """
    Remove all files and folders in 'args.site' except for those named in 'keep'
    """

    for item in (args.site).iterdir():
        if item.name not in keep:
            if item.is_file() or item.is_symlink():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)


def emit_versions(args):
    """
    Emits a versions.json in the root of args.site

    Conventions
    ===========

    * The preferred version is always set to be the latest version
    """

    dirs = (re.match(REGEX_VERSION_TAG, d.name) for d in (args.site / "en").iterdir())
    matches = ((d.group(1), d.group(2), d.group(3)) for d in dirs if d)
    semvers = [".".join(semver) for semver in sorted(matches, reverse=True)]
    latest = semvers[0]

    versions = [
        {
            "name": "next",
            "url": f"{args.url}/en/next",
        }
    ]
    for semver in semvers:
        version = {
            "version": f"v{semver}",
            "name": f"v{semver}",
            "url": f"{args.url}/en/v{semver}",
        }
        if semver == latest:
            version["preferred"] = True
            version["name"] = f"v{latest} (latest)"
            version["url"] = f"{args.url}/"

        versions.append(version)

    with (args.site / "versions.json").open(mode="w", encoding="utf-8") as file:
        json.dump(versions, file, indent=4, ensure_ascii=False)

    return latest


def check_ref(args):
    """
    Verify that args.ref matches assumptions.
    When matching, then return the short-name, when it does not, then return None.
    """

    is_tag = "tags" in args.ref
    is_branch = "heads" in args.ref
    if not (is_tag or is_branch):
        print(f"Failed: ref('{args.ref}') is neither tag nor branch")
        return None
    if "/" not in args.ref or len(args.ref.split("/")) < 3:
        print(f"Failed: ref('{args.ref}') has unexpected format")
        return None

    return "-".join(args.ref.split("/")[2:])


def main(args):
    """
    Add the given 'args.docs' to 'args.site/en/<ref>'
    For tags also add to 'args.site/en/latest'
    In both cases existing docs are removed
    """

    ref = check_ref(args)
    if not ref:
        return 1

    # Clean the github-pages repository
    remove_except(args, KEEPLIST)

    # Remove existing instance named 'ref'
    ref_path = args.site / "en" / ref
    if ref_path.exists():
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    # Add 'ref' instance to site
    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, args.site / "en" / ref)

    # Add 'versions.json' and return the latest version
    latest = emit_versions(args)

    print(ref, f"v{latest}")

    # Update the site-root
    if ref == "docs" or ref == f"v{latest}":
        print(f"Copying from: '{ref}' to '{args.site}'")
        shutil.copytree(ref_path, args.site / ".", dirs_exist_ok=True)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
