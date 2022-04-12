#!/usr/bin/env python3
"""
Extract table captions from a page in the NVMe specification
"""

from itertools import pairwise
import subprocess
import xml.etree.ElementTree as ET


def extract_captions(input_file, pages):
    """Extract captions from page"""
    page = pages.split("-")[0]
    xml = subprocess.run(["pdftohtml", "-f", page, "-l", page, "-xml", "-stdout", "-i",input_file], stdout=subprocess.PIPE, text=True)
    tree = ET.fromstring(xml.stdout)
    elements = tree.findall(".//text[@font='2'][b]")
    return list(
        yield_text(
            remove_non_figure_text(
                concat_dashed_elements(
                    remove_empty_elements(
                        flatten_bold_elements(elements))))))


def flatten_bold_elements(elements):
    """Make parent include text from a bold child"""
    for element in elements:
        child = element.find(".//b")
        element.text = child.text
        yield element


def remove_empty_elements(elements):
    """Remove any elements with no text"""
    for element in elements:
        if element.text.strip():
            yield element


def remove_non_figure_text(elements):
    """Remove any elements that doesn't include 'Figure'"""
    for element in elements:
        if element.text.startswith("Figure"):
            yield element


def yield_text(elements):
    """Yield only the text of each element"""
    for element in elements:
        yield element.text


def concat_dashed_elements(elements):
    """Remove false linebreaks caused by a dash"""
    skip = False
    for first, second in pairwise(elements):
        second_left = int(second.get("left"))
        first_right = int(first.get("left")) + int(first.get("width"))
        if 0 <= second_left - first_right < 3:
            first.text += second.text
            skip = True
            yield first
        elif not skip:
            yield first
        else:
            skip = False


def main(input_file, pages):
    """Entry point"""
    try:
        return extract_captions(input_file, pages)
    except FileNotFoundError:
        return {}
