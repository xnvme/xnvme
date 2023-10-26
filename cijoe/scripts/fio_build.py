#!/usr/bin/env python3
"""
build and install fio
=====================

Build fio in fio.repository.path, using prefix fio.build.prefix.

.. note::
    This script does not decide **which** version of fio to build and install,
    it simply consumes the one at  'fio.repository.path'. However, for
    convenience, then the script outputs the current git ref for HEAD, also the
    'make clean' of fio is quite descriptive as well.

Retargetable: True
------------------
"""
from pathlib import Path


def main(args, cijoe, step):
    """Configure, build and install fio in 'config.options.fio.build.prefix'"""

    commands = [
        "git rev-parse --short HEAD",
        "make clean",
        f"./configure --prefix={ cijoe.config.options['fio']['build']['prefix'] }",
        "make -j $(nproc)",
        "make install",
    ]
    for cmd in commands:
        err, _ = cijoe.run(
            cmd, cwd=Path(cijoe.config.options["fio"]["repository"]["path"])
        )
        if err:
            return err

    return err
