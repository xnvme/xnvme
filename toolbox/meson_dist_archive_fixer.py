#!/usr/bin/env python3

# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

"""
    Removes '.git' and SPDK build artifacts from subprojects

    This script is intended to be added with 'meson.add_dist_script()', to cleanup the
    subprojects, that is, remove the '.git' directories and SPDK build artifacts
    (build/ directory and mk/config.mk) that contain host-specific absolute paths.
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

    for root, dnames, fnames in os.walk(args.path, topdown=True):
        for dname in dnames[:]:
            path = os.path.join(root, dname)
            if "builddir" not in path or "subprojects" not in path:
                continue
            if dname == ".git":
                shutil.rmtree(path)
                dnames.remove(dname)
            elif dname == "build" and os.path.join(os.sep, "spdk", "build") in path:
                shutil.rmtree(path)
                dnames.remove(dname)
        for fname in fnames:
            path = os.path.join(root, fname)
            if "builddir" not in path or "subprojects" not in path:
                continue
            if (
                fname == "config.mk"
                and os.path.join(os.sep, "spdk", "mk", "config.mk") in path
            ):
                os.remove(path)

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
