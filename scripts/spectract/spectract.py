#!/usr/bin/env python3
"""
Extract content of NVMe specification table and convert it to yaml
"""
import argparse
import os
import sys

import spectract_scheduler
import spectract_parser


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def process_parser_args(args):
    """Process the arguments and enforce defaults if values are undefined"""
    input_file = expand_path(args.input)
    pages = args.pages
    if args.output is None:
        filename = os.path.splitext(os.path.basename(args.input))[0]
        output_file = f'{filename}_{args.pages}_{args.tables}.yaml'
    else:
        output_file = args.output
    table_indices = [int(i) for i in args.tables.split('-')]
    return input_file, pages, table_indices, output_file


def parser(args):
    input_file, pages, table_indices, output_file = process_parser_args(args)
    spectract_parser.main(input_file, pages, table_indices, output_file)


def scheduler(args):
    spectract_scheduler.main(expand_path(args.file))


def main():
    """Parse command-line arguments and call relevant function"""
    prsr = argparse.ArgumentParser(
        description='Extract content from NVMe specification and convert it to yaml',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    subprsrs = prsr.add_subparsers()

    subprsr_scheduler = subprsrs.add_parser('multiple',
                                            help='Extract multiple tables by defining targets in a yaml file')
    subprsr_scheduler.add_argument(
        'file',
        help='The yaml file containing the targets to extract from',
    )

    subprsr_scheduler.set_defaults(func=scheduler)

    subprsr_parser = subprsrs.add_parser('single',
                                         help='Extract a single table with command line arguments')
    subprsr_parser.add_argument(
        '--input', '-i',
        help='The file to extract the table from',
        required=True
    )
    subprsr_parser.add_argument(
        '--pages', '-p',
        help='The pages to extract the table from, input a string with format 1 or 1-3',
        required=True
    )
    subprsr_parser.add_argument(
        '--tables', '-t',
        help='The indices of the desired tables in the page range, input a string with format 0 or 0-3',
        required=True
    )

    subprsr_parser.add_argument(
        '--output', '-o',
        help='The file to write the result to, defaults to <input filename>_<pages>_<tables>.yaml',
        required=False,
    )

    subprsr_parser.set_defaults(func=parser)

    args = prsr.parse_args()

    args.func(args)


if __name__ == '__main__':
    sys.exit(main())
