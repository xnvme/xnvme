#!/usr/bin/env python3
"""
Schedule multiple instances of the spectract parser from a yaml file
"""
import concurrent.futures
import argparse
import os
import sys
import yaml

import spectract_parser


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def setup():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description='Schedule multiple instances of the spectract parser from a yaml file',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        'file',
        help='The yaml file containing the targets to parse',
    )

    args = prsr.parse_args()
    args.file = expand_path(args.file)

    return args


def process_args(target, args):
    """Assign the values from the target to args and return args"""
    args.input = target['input']
    args.pages = str(target['pages'])
    args.tables = str(target['tables'])
    if 'output' not in target:
        args.output = None

    return spectract_parser.process_args(args)


def schedule(targets, args):
    """Assign each target to a different process"""
    executor = concurrent.futures.ProcessPoolExecutor(10)
    futures = [executor.submit(parse, target, args) for target in targets]
    concurrent.futures.wait(futures)


def parse(target, args):
    """Pass args to the parser"""
    spectract_parser.main(process_args(target, args))


def main(args):
    """Entry point"""

    try:
        with open(args.file, 'r') as stream:
            try:
                schedule(yaml.safe_load(stream), args)
            except yaml.YAMLError as err:
                print(err)
                return 1
    except FileNotFoundError:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(setup()))
