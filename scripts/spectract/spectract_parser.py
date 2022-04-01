#!/usr/bin/env python3
"""
Extract content of NVMe specification table and convert it to yaml
"""
import camelot
import pandas
import re


def extract_table(input_file, pages, table_indices):
    """Read tables, remove first row of each and concatenate"""

    tables = camelot.read_pdf(input_file, pages)
    headings = extract_headings(tables[table_indices[0]].df)

    if len(table_indices) == 1:
        table = tables[table_indices[0]].df.drop(0).reset_index(drop=True)
    else:
        table = pandas.concat([t.df.drop(0)
                               for t in
                               tables[table_indices[0]:table_indices[1]+1]]).reset_index(drop=True)
    return extract_content(headings, table)


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


def extract_content(headings, table):
    """Extract content from table"""
    output = []
    subheadings = None
    for row in table.itertuples(index=False, name=None):
        if row[0] != '':
            output.append(extract_row(row, headings, 0))

            # reset subheadings
            subheadings = None

        # Extract from nested tables
        elif row[len(headings)] != '':
            if subheadings is None and row[len(headings)].lower() in ['bits','bytes','value','code']:
                subheadings = [h.lower().replace('\n', '')
                               for h in row
                               if h not in [None, '']]
            elif subheadings:
                output[-1]['children'].append(extract_row(row, subheadings, len(headings)))
            else:
                output[-1]['children'].append(extract_row(row, headings, len(headings)))

    return output


def main(input_file, pages, table_indices):
    """Entry point"""
    try:
        return extract_table(input_file, pages, table_indices)
    except FileNotFoundError:
        return {}
