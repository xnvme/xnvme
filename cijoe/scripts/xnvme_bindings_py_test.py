#!/usr/bin/env python3
"""
Test xNVMe bindings for Python
===============================

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
    """Test xNVMe"""

    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    err, _ = cijoe.run("pytest test_buf.py", cwd=xnvme_source / "python" / "tests")
    if err:
        return err

    return 0
