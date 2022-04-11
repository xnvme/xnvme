#!/usr/bin/env python3
"""
Transform and clean up tables
"""


class Table(list):

    def __init__(self, name, content):
        self.name = name
        self.content = content
        self.headings = content[0].keys()

    def print_rows(self):
        for row in self.content:
            print(row)

    def remove_value_ranges(self):
        if "value" in self.headings:
            self.content = [row for row in self.content if "to" not in row["value"]]

    def remove_empty_children(self):
        for row in self.content:
            if not row["children"]:
                del row["children"]

    def generate_name(self):
        if "name" not in self.headings:
            for row in self.content:
                name = row["definition"] if "definition" in row.keys() else row["description"]
                row["name"] = name.upper().replace(" ", "_")

    def convert_values_to_hex(self):
        if "value" in self.headings:
            for row in self.content:
                row["value"] = "0x" + row["value"].strip("h")


def transform(tables):
    transformed = {}
    for name, content in tables.items():
        table = Table(name, content)
        table.remove_empty_children()
        table.remove_value_ranges()
        table.generate_name()
        table.convert_values_to_hex()
        transformed[table.name] = table.content

    return transformed


def main(tables):
    """Entry point"""
    return transform(tables)
