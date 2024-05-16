#!/usr/bin/env python3
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
    """Parse command-line arguments for cij_extractor"""

    # Parse the Command-Line
    prsr = argparse.ArgumentParser(
        description="cij_extractor - CIJOE Test data extractor",
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
