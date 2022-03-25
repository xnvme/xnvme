#!/usr/bin/env python3
"""
Extract content of NVMe specification table and convert it to yaml
"""
import camelot
import pandas
import yaml
import re
import argparse
import os
import sys


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def process_args(args):
    """Process the arguments and enforce defaults if values are undefined"""
    args.input = expand_path(args.input)
    if args.output is None:
        filename = os.path.splitext(os.path.basename(args.input))[0]
        args.output = f'{filename}_{args.pages}_{args.tables}.yaml'

    args.tables = [int(i) for i in args.tables.split('-')]
    return args


def setup():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description='Extract content of NVMe specification table and convert to yaml',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        '--input', '-i',
        help='The file to extract the table from',
        required=True
    )
    prsr.add_argument(
        '--pages', '-p',
        help='The pages to extract the table from, input a string with format 1 or 1-3',
        required=True
    )
    prsr.add_argument(
        '--tables', '-t',
        help='The indices of the desired tables in the page range, input a string with format 0 or 0-3',
        required=True
    )

    prsr.add_argument(
        '--output', '-o',
        help='The file to write the result to, defaults to <input filename>_<pages>_<tables>.yaml',
        required=False,
    )
    args = prsr.parse_args()
    args = process_args(args)
    return args


def extract_table(args):
    """Read tables, remove first row of each and concatenate"""

    tables = camelot.read_pdf(args.input, args.pages)
    args.headings = extract_headings(tables[args.tables[0]].df)

    if len(args.tables) == 1:
        args.table = tables[args.tables[0]].df.drop(0).reset_index(drop=True)
    else:
        args.table = pandas.concat([t.df.drop(0)
                                    for t in
                                    tables[args.tables[0]:args.tables[1]+1]]).reset_index(drop=True)
    return args


def extract_headings(table):
    """Extract headings from first row of table given as a dataframe"""

    return [h.lower().replace('\n', '')
            for row in table.head(1).to_numpy()
            for h in row
            if h not in [None, '']]


def extract_row(row, headings, start_index):
    """Extract content from row given as a tuple"""
    content = {'children': []}

    name_brief_verbose_regex = re.compile('(?P<brief>.+?)\((?P<name>.+?)\):(?P<verbose>.+)')
    brief_verbose_regex = re.compile('(?P<brief>.+?):(?P<verbose>.+)')

    for i, h in enumerate(headings):
        if h in ['bits', 'bytes']:
            content[h] = [int(b) for b in row[start_index+i].split(':')]

        elif match := name_brief_verbose_regex.match(row[start_index + i].replace('\n', '')):
            content['name'] = match.group('name').strip().lower()
            content['brief'] = match.group('brief').strip()
            content['verbose'] = match.group('verbose').strip()

        elif match := brief_verbose_regex.match(row[start_index + i].replace('\n', '')):
            content['brief'] = match.group('brief').strip()
            content['verbose'] = match.group('verbose').strip()

        else:
            content[h] = row[start_index+i]

    return content


def extract_content(args):
    """Extract content from table"""
    output = []
    subheadings = None
    for row in args.table.itertuples(index=False, name=None):
        if row[0] != '':
            output.append(extract_row(row, args.headings, 0))

            # reset subheadings
            subheadings = None

        # Extract from nested tables
        elif row[len(args.headings)] != '':
            if subheadings is None and row[len(args.headings)].lower() in ['bits','bytes','value','code']:
                subheadings = [h.lower().replace('\n', '')
                               for h in row
                               if h not in [None, '']]
            elif subheadings:
                output[-1]['children'].append(extract_row(row, subheadings, len(args.headings)))
            else:
                output[-1]['children'].append(extract_row(row, args.headings, len(args.headings)))

    return output


def write_to_yaml(output, filename):
    """Write output to yaml"""
    with open(filename, 'w') as file:
        yaml.dump(output, file, default_flow_style=None)


def main(args):
    """Entry point"""
    try:
        write_to_yaml(extract_content(extract_table(args)), args.output)
    except FileNotFoundError:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(setup()))
