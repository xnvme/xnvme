#!/usr/bin/env python3
"""
Build xNVMe from source
=======================

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
    """Build xNVMe"""

    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    os_name = cijoe.getconf("os.name", "")

    commands = [
        "git rev-parse --short HEAD || true",
        "rm -r builddir || true",
        (
            'meson setup builddir --prefix "C:/tools/msys64/usr" --default-library=static'
            if os_name == "windows"
            else "meson setup builddir"
        ),
        "meson compile -C builddir",
        "cat builddir/meson-logs/meson-log.txt",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source)
        if err and not first_err:
            first_err = err

    return first_err
