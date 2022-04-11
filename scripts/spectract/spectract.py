#!/usr/bin/env python3
"""
Extract content of NVMe specification tables and generate C header
"""
import argparse
import os
import sys

import spectract_generator
import spectract_scheduler


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def scheduler(args):
    spectract_scheduler.main(expand_path(args.file))


def generator(args):
    spectract_generator.main(expand_path(args.file))


def main():
    """Parse command-line arguments and call relevant function"""
    prsr = argparse.ArgumentParser(
        description="Extract content from NVMe specification and generate C header",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    subprsrs = prsr.add_subparsers()

    subprsr_scheduler = subprsrs.add_parser(
        "extract", help="Extract tables by defining targets in a yaml file"
    )

    subprsr_scheduler.add_argument(
        "file",
        help="The yaml file containing the targets to extract",
    )

    subprsr_scheduler.set_defaults(func=scheduler)

    subprsr_generator = subprsrs.add_parser(
        "generate", help="Generate C headers from a yaml file"
    )

    subprsr_generator.add_argument(
        "file",
        help="The yaml file containing the tables to generate the headers from",
    )

    subprsr_generator.set_defaults(func=generator)

    args = prsr.parse_args()

    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
