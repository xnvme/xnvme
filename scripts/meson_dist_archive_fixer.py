#!/usr/bin/env python3
"""
    Removes '.git' from subprojects

    This script is intended to be added with 'meson.add_dist_script()', to cleanup the
    subprojects, that is, remove the '.git'.
"""
import argparse
import os
import shutil
import sys


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def parse_args():
    """Parse arguments for dist-archive-fixer"""

    prsr = argparse.ArgumentParser(
        description="Meson dist-archive fixer",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--path",
        default=os.getcwd(),
        help="Path to 'meson dist' generated zip-file",
    )

    return prsr.parse_args()


def main(args):
    """Main entry point"""

    if not (args.path.endswith("builddir") and os.path.exists("meson-dist")):
        print("ERR: path: '{path}' looks bad".format(path=args.path))
        return 1

    for root, dnames, _ in os.walk(args.path):
        for dname in dnames:
            path = os.path.join(root, dname)
            if dname == ".git" and "builddir" in path and "subprojects" in path:
                shutil.rmtree(path)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
