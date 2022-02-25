#!/usr/bin/env python3
"""Traverse Makefile and print all help instructions"""
import re
import argparse
import sys
import os


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def setup():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description="Traverse Makefile and print all help instructions",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        "--repos",
        help="Path to root of the xNVMe repository",
        required=True
    )
    prsr.add_argument(
        "--verbose",
        action="store_true",
        help="If set, print verbose descriptions",
        required=False
    )
    args = prsr.parse_args()
    args.repos = expand_path(args.repos)

    return args


def gen_help(args):
    """Generate dict from help instructions in Makefile"""
    define_regex = re.compile("define (?P<target>.+)-help")
    endef_regex = re.compile("endef")
    args.help = {}
    key = ""
    desc = []
    with open(os.path.join(args.repos, 'Makefile')) as makefile:
        for line in makefile.readlines():
            if define_regex.match(line):
                key = define_regex.match(line).group('target')
            elif endef_regex.match(line):
                args.help[key] = [ln[1:].strip("\n") for ln in desc
                                  if ln.startswith("#")]
                key = ""
                desc = []
            elif key:
                desc.append(line)
    return args


def print_help(args):
    """Print the help instructions"""
    for key, desc in sorted(args.help.items()):
        print(key + ":")
        if args.verbose:
            for line in desc:
                print("\t" + line)
        else:
            print("\t" + desc[0])


def main(args):
    """Entry point"""

    try:
        print_help(gen_help(args))
    except FileNotFoundError:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(setup()))
