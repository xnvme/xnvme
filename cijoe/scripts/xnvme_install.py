#!/usr/bin/env python3
"""
Install xNVMe from previously built source
==========================================

Retargetable: True
------------------
"""
import errno
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default=None,
        help="path to xNVMe source (default: config.xnvme.repository.path)",
    )


def main(args, cijoe):
    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    commands = [
        "meson compile",
        "meson install",
        "meson --internal uninstall",
        "meson install",
        "cat meson-logs/meson-log.txt",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=f"{xnvme_source}/builddir")
        if err:
            return err

    return 0
