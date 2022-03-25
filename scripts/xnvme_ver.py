#!/usr/bin/env python3
"""
    Extract the version meson build

    When running from shell, returns 0 on success, some other value otherwise
"""
import argparse
import os
import sys


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def xnvme_ver(path=None):
    """
    Retrieve the version from project CMakeLists.txt

    @returns "x.y.z", {"major": x, "minor": y, "patch": z}
    """

    if path is None:
        path = os.sep.join(["..", "..", "meson.build"])

    with open(path) as cmake:
        for line in cmake.readlines():
            if "version:" not in line:
                continue

            _, vtxt = line.split("version:", 1)

            return vtxt.replace(",", "").replace("'", "").strip()

    return ""


def parse_args():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description="Extract the xNVMe version from the meson.build",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument("--path", help="Path to 'meson.build'", required=True)
    args = prsr.parse_args()
    args.path = expand_path(args.path)

    return args


def main(args):
    """Entry point"""

    try:
        print(xnvme_ver(args.path))
    except FileNotFoundError:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(parse_args()))
