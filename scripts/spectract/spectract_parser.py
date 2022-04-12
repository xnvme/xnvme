#!/usr/bin/env python3
"""
Extract content of NVMe specification table and convert it to yaml
"""

import re

import camelot
import pandas

import spectract_captions


def extract_table(input_file, pages, table_indices):
    """Read tables, remove first row of each and concatenate"""

    tables = camelot.read_pdf(input_file, pages, line_scale=35)
    captions = spectract_captions.main(input_file, pages)
    assert(len(tables) == len(captions))

    caption = captions[table_indices[0]]
    headings = extract_headings(tables[table_indices[0]].df.head(1).to_numpy()[0])

    if len(table_indices) == 1:
        table = tables[table_indices[0]].df.drop(0).reset_index(drop=True)
    else:
        table = pandas.concat(
            [t.df.drop(0) for t in tables[table_indices[0] : table_indices[1] + 1]]
        ).reset_index(drop=True)
    return {caption: extract_content(headings, table)}


def extract_headings(row):
    """Extract headings from row of table given as a dataframe"""

    headings = [h.lower().replace("\n", "") for h in row if h not in [None, ""]]

    # clean-up headings containing value
    if "value" in headings[0]:
        headings[0] = "value"

    return headings


def extract_row(row, headings, start_index):
    """Extract content from row given as a tuple"""
    content = {"children": []}

    name_brief_verbose_regex = re.compile(
        r"(?P<brief>.+?)\((?P<name>.+?)\):(?P<verbose>.+)"
    )
    brief_verbose_regex = re.compile(r"(?P<brief>.+?):(?P<verbose>.+)")

    for i, heading in enumerate(headings):
        if heading in ["bits", "bytes"]:
            content[heading] = [int(b) for b in row[start_index + i].split(":")]

        elif match := name_brief_verbose_regex.match(
            row[start_index + i].replace("\n", "")
        ):
            content["name"] = match.group("name").strip().lower()
            content["brief"] = match.group("brief").strip()
            content["verbose"] = match.group("verbose").strip()

        elif match := brief_verbose_regex.match(row[start_index + i].replace("\n", "")):
            content["brief"] = match.group("brief").strip()
            content["verbose"] = match.group("verbose").strip()

        else:
            content[heading] = row[start_index + i]

    return content


def extract_content(headings, table):
    """Extract content from table"""
    output = []
    subheadings = None
    for row in table.itertuples(index=False, name=None):
        if row[0] != "":
            output.append(extract_row(row, headings, 0))

            # reset subheadings
            subheadings = None

        # Extract from nested tables
        elif row[len(headings)] != "":
            if subheadings is None and row[len(headings)].lower() in [
                "bits",
                "bytes",
                "value",
                "code",
            ]:
                subheadings = extract_headings(row)
            elif subheadings:
                output[-1]["children"].append(
                    extract_row(row, subheadings, len(headings))
                )
            else:
                output[-1]["children"].append(extract_row(row, headings, len(headings)))

    return output


def main(input_file, pages, table_indices):
    """Entry point"""
    try:
        return extract_table(input_file, pages, table_indices)
    except FileNotFoundError:
        return {}
