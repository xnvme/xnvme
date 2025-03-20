#!/usr/bin/env python3
"""
Clean xNVMe from source / repository
====================================

Retargetable: True
------------------
"""
import errno
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default=None,
        help="path to xNVMe source (default: config.xnvme.repository.path)",
    )


def main(args, cijoe):
    """Clean xNVMe"""

    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    commands = [
        "make clean",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source)
        if err and not first_err:
            first_err = err

    return first_err
