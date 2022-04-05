#!/usr/bin/env python3
"""
Schedule multiple instances of the spectract parser from a yaml file
Collect results and write them to yaml
"""
import concurrent.futures
import os

import spectract_parser
import yaml


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def process_target(target):
    """Process the values from the target and return them"""
    input_file = expand_path(target["input"])
    pages = str(target["pages"])
    table_indices = [int(i) for i in str(target["tables"]).split("-")]
    name = target["name"]
    return input_file, pages, table_indices, name


def schedule(targets):
    """Assign each target to a different process and collect results in dict"""
    with concurrent.futures.ProcessPoolExecutor(10) as executor:
        tables = {}
        for table in executor.map(parse, targets):
            tables.update(table)
        return tables


def parse(target):
    """Pass args to the parser"""
    input_file, pages, table_indices, name = process_target(target)
    return {name: spectract_parser.main(input_file, pages, table_indices)}


def write_to_yaml(output):
    """Write output to yaml"""
    with open("output.yaml", "w") as file:
        yaml.dump(output, file, default_flow_style=None)


def main(yaml_file):
    """Entry point"""
    try:
        with open(yaml_file, "r") as stream:
            try:
                write_to_yaml(schedule(yaml.safe_load(stream)))
            except yaml.YAMLError as err:
                print(err)
                return 1
    except FileNotFoundError:
        return 1

    return 0
