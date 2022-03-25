#!/usr/bin/env python3
import argparse
import os
import shutil
import sys


def parse_args():
    """Parse command-line arguments for cij_extractor"""

    # Parse the Command-Line
    prsr = argparse.ArgumentParser(
        description="cij_extractor - CIJOE Test data extractor",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--docs", help="Path SphinxDoc generated HTML", default=os.getcwd()
    )
    prsr.add_argument(
        "--site",
        help="Path to xNVMe.io GitHUB Repository",
        default=os.getcwd(),
    )
    prsr.add_argument("--ref", help="xNVMe repository reference", default=os.getcwd())
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

    ref_path = os.path.join(args.site, "docs", ref)
    if os.path.exists(ref_path):
        print(f"Removing: '{ref_path}'")
        shutil.rmtree(ref_path)

    print(f"Copying from: '{args.docs}' to '{ref_path}'")
    shutil.copytree(args.docs, ref_path)

    if not is_tag:
        return 0

    latest_path = os.path.join(args.site, "docs", "latest")
    if os.path.exists(latest_path):
        print(f"Removing: '{latest_path}'")
        shutil.rmtree(latest_path)

    print(f"Copying from: '{args.docs}' to '{latest_path}'")
    shutil.copytree(args.docs, latest_path)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
