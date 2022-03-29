#!/usr/bin/env python3
"""
Schedule multiple instances of the spectract parser from a yaml file
"""
import concurrent.futures
import yaml
import os

import spectract_parser


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def process_target(target):
    """Process the values from the target and return them"""
    input_file = expand_path(target['input'])
    pages = str(target['pages'])
    table_indices = [int(i) for i in str(target['tables']).split('-')]
    if 'output' not in target:
        filename = os.path.splitext(os.path.basename(input_file))[0]
        output_file = f'{filename}_{pages}_{target["tables"]}.yaml'
    else:
        output_file = target['output']

    return input_file, pages, table_indices, output_file


def schedule(targets):
    """Assign each target to a different process"""
    executor = concurrent.futures.ProcessPoolExecutor(10)
    futures = [executor.submit(parse, target) for target in targets]
    concurrent.futures.wait(futures)


def parse(target):
    """Pass args to the parser"""
    input_file, pages, table_indices, output_file = process_target(target)
    spectract_parser.main(input_file, pages, table_indices, output_file)


def main(yaml_file):
    """Entry point"""
    try:
        with open(yaml_file, 'r') as stream:
            try:
                schedule(yaml.safe_load(stream))
            except yaml.YAMLError as err:
                print(err)
                return 1
    except FileNotFoundError:
        return 1

    return 0
