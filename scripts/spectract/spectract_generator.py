#!/usr/bin/env python3
"""
Generate C headers from a yaml file
"""
import yaml

import spectract_transformer

comment_start = r"/**"
comment_end = r"*/"
curly_start = r"{"
curly_end = r"};"


def print_lines(lines):
    for line in lines:
        print(line)


def gen_doc(table, table_name, table_type):
    """Generate docstrings from table"""
    lines = []
    lines.append(comment_start)
    for row in table:
        row_name = row["name"]
        row_description = row["description"]
        lines.append(f"* @var {row_name} {row_description}")

    lines.append(f"* @{table_type} {table_name}")
    lines.append(comment_end)
    return lines


def gen_enum(table_name, table):
    """Generate an enum from table"""
    doc = gen_doc(table, table_name, "enum")

    code = []
    code.append(f"enum {table_name} " + curly_start)
    for row in table:
        row_name = row["name"]
        row_value = row["value"]
        code.append(f"{row_name} = {row_value},")
    code.append(curly_end)
    return doc + code


def generate(tables):
    """Generate C headers from tables"""
    for table_name, table in tables.items():
        yield gen_enum(table_name, table)


def write_header(tables):
    """Write to header file"""
    with open("header.h", "w") as file:
        file.write("%s\n" % comment_start)
        file.write("* @headerfile header.h\n")
        file.write("%s\n" % comment_end)
        for table in generate(tables):
            for line in table:
                file.write("%s\n" % line)


def main(yaml_file):
    """Entry point"""
    try:
        with open(yaml_file, 'r') as stream:
            try:
                tables = spectract_transformer.main(yaml.safe_load(stream))
            except yaml.YAMLError as err:
                print(err)
                return 1
        write_header(tables)
    except FileNotFoundError:
        return 1

    return 0
