#!/usr/bin/env python3
import argparse
import shutil
import sys
from pathlib import Path


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


def main(args):
    """
    Add the given 'args.docs' to 'args.site/docs/<ref>'
    For tags also add to 'args.site/docs/latest'
    In both cases existing docs are removed
    """

    is_tag = "tags" in args.ref
    is_branch = "heads" in args.ref
    if not (is_tag or is_branch):
        print(f"Failed: ref('{args.ref}') is neither tag nor branch")
        return 1
    if "/" not in args.ref or len(args.ref.split("/")) < 3:
        print(f"Failed: ref('{args.ref}') has unexpected format")
        return 1

    ref = "-".join(args.ref.split("/")[2:])

    ref_path = Path(args.site) / "docs" / ref
    if ref_path.exists():
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, ref_path)

    if not is_tag:
        return 0

    latest_path = Path(args.site) / "docs" / "latest"
    if latest_path.exists():
        print(f"Removing: '{latest_path}'")
        shutil.rmtree(latest_path)

    print(f"Copying from: '{args.docs}' to '{latest_path}'")
    shutil.copytree(args.docs, latest_path)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
