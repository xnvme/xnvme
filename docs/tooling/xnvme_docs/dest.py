#!/usr/bin/env python3
"""
xNVMe Documentation Deployment to GitHub Pages

This script organizes the website and documentation for xnvme.io.

When running ``xnvme-docs-build-html`` inside the xNVMe repository, a documentation
instance is emitted at ``docs/builddir/html``. This path is given to the script
via ``--docs``.

The documentation/website lives in a repository dedicated to GitHub Pages.
A path to this repository is given to the script via ``--site``.

The intent of the organization is two-fold:

* Provide the documentation of the ``main`` branch as the default site at
  ``https://xnvme.io/``, since ``main`` holds the latest state of xNVMe.
  Other in-development branches are not available via version-switcher,
  however, they can be pointed to manually e.g. https://xnvme.io/en/feature-a

* Keep a bit of history on the project documentation available online,
  e.g. for those on an older tagged release.

Supported refs are on the form:
    refs/heads/<branch_name>
    refs/tags/<tag_name>

The organization is as follows: for all refs, the folder ``--docs`` is
renamed to ``--ref`` and placed in ``--site/en/<ref>``. Examples:
    <site>/en/main
    <site>/en/v1.2.3

The site root (``<site>/``) is a copy of ``<site>/en/main`` so that
``https://xnvme.io/`` serves the latest state of xNVMe.

In addition to the things emitted by sphinx-doc, there are a couple of files
specific to GitHub Pages, things like ``.nojekyll``, a README.md, etc.
These files and the folder ``docs`` are recorded in a ``KEEPLIST``.

This organization is done by:
1. Removing everything in ``--site`` that is not in KEEPLIST
2. Copy ``--docs`` to ``--site/en/<ref>``
3. Emit a PyData versions.json docs in ``--site``
   - Note: The script organizes instances of the documentation that are not
     exposed in the PyData versions.json
   - The instances exposed are ``main`` and each version-tag (vMAJOR.MINOR.PATCH)
   - The preferred version is ``main``
4. Copy ``<site>/en/main`` to the site root

It is left to another script to commit/push the GitHub pages repository.
"""
from __future__ import annotations

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


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments for docs destination script."""
    prsr = argparse.ArgumentParser(
        description="xNVMe documentation organizer for GitHub Pages",
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
        help="xNVMe repository reference (e.g. refs/heads/main or refs/tags/v1.2.3)",
    )
    prsr.add_argument(
        "--url",
        help="Base URL for versions.json",
        type=str,
        default="https://xnvme.io",
    )
    args = prsr.parse_args()
    args.docs = args.docs.resolve()
    args.site = args.site.resolve()

    return args


def remove_except(site: Path, keep: list[str]) -> None:
    """
    Remove all files and folders in 'site' except for those named in 'keep'.

    Args:
        site: Path to the site directory.
        keep: List of names to keep.
    """
    for item in site.iterdir():
        if item.name not in keep:
            if item.is_file() or item.is_symlink():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)


def emit_versions(site: Path, url: str) -> None:
    """
    Emit a versions.json in the root of site.

    Conventions:
    - ``main`` is listed first with ``version: "main"`` so the PyData
      version-switcher can match against it and render it as a
      selectable entry.
    - The latest tagged release is marked ``preferred: true``. PyData
      uses ``preferred`` to drive the "switch to stable version"
      warning banner -- non-preferred non-main views (older tags) get
      the banner; main and the latest tag do not.
    - Each tagged release is listed as ``vX.Y.Z`` pointing at
      ``/en/vX.Y.Z``.

    Args:
        site: Path to the site directory.
        url: Base URL for the versions.
    """
    dirs = (re.match(REGEX_VERSION_TAG, d.name) for d in (site / "en").iterdir())
    matches = ((d.group(1), d.group(2), d.group(3)) for d in dirs if d)
    semvers = [".".join(semver) for semver in sorted(matches, reverse=True)]

    versions: list[dict[str, str | bool]] = [
        {
            "name": "main",
            "version": "main",
            "url": f"{url}/",
        }
    ]
    for index, semver in enumerate(semvers):
        entry: dict[str, str | bool] = {
            "version": f"v{semver}",
            "name": f"v{semver}",
            "url": f"{url}/en/v{semver}",
        }
        if index == 0:
            entry["preferred"] = True
        versions.append(entry)

    with (site / "versions.json").open(mode="w", encoding="utf-8") as file:
        json.dump(versions, file, indent=4, ensure_ascii=False)


def check_ref(ref: str) -> str | None:
    """
    Verify that ref matches assumptions.

    Args:
        ref: The git reference string.

    Returns:
        Short name when matching, None when it does not.
    """
    is_tag = "tags" in ref
    is_branch = "heads" in ref
    if not (is_tag or is_branch):
        print(f"Failed: ref('{ref}') is neither tag nor branch")
        return None
    if "/" not in ref or len(ref.split("/")) < 3:
        print(f"Failed: ref('{ref}') has unexpected format")
        return None

    return "-".join(ref.split("/")[2:])


def main() -> int:
    """
    Add the given '--docs' to '--site/en/<ref>' and refresh the site root.

    The site root is always a copy of '<site>/en/main'.

    Returns:
        Exit code (0 for success, non-zero for failure).
    """
    args = parse_args()

    ref = check_ref(args.ref)
    if not ref:
        return 1

    # Clean the github-pages repository
    remove_except(args.site, KEEPLIST)

    # Remove existing instance named 'ref'
    ref_path = args.site / "en" / ref
    if ref_path.exists():
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    # Add 'ref' instance to site
    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, args.site / "en" / ref)

    # Emit versions.json
    emit_versions(args.site, args.url)

    # Insert site at the web-root: always 'main'
    main_path = args.site / "en" / "main"
    if main_path.exists():
        print(f"Copying from: '{main_path}' to '{args.site}'")
        shutil.copytree(
            main_path,
            args.site / ".",
            dirs_exist_ok=True,
        )
    else:
        print(f"Skipping site-root refresh: '{main_path}' does not exist yet")

    return 0


if __name__ == "__main__":
    sys.exit(main())
