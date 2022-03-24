#!/usr/bin/env python3
"""
This script is the equivalent use of 'sed -i', for the specific case of replace
ascii-codes-format-codes
'"""

chars = [
    "\033[1m",
    "\033[0m",
]


def main():

    with open("00_make.out") as ofile:
        content = ofile.read()
        for char in chars:
            content = content.replace(char, "")

    with open("make.out", "w") as ofile:
        ofile.write(content)


if __name__ == "__main__":
    main()
