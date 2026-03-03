#!/usr/bin/env python3
"""
Build xNVMe bindings for Python
===============================

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
    """Build xNVMe"""

    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    commands = []

    err, _ = cijoe.run("apt-get --version")
    if not err:
        commands += ["apt-get install libclang-dev -qy"]

    commands += [
        "pipx ensurepath",
        "make uninstall",
        "make clean",
        "make build",
        "make deps",
        "make install",
        "make test",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=str(Path(xnvme_source) / "python" / "bindings"))
        if err:
            return err

    return 0
