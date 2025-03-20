#!/usr/bin/env python3
"""
Build preparation: install xNVMe package dependencies
=====================================================

Configuration Requirements
--------------------------

Needs an ``os`` key describing the operating system by name and version, example::

    os:
      name: 'debian'
      version: 'bullseye'

Retargetable: True
------------------
"""
import errno
import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default=None,
        help="path to xNVMe source (default: config.xnvme.repository.path)",
    )


def main(args, cijoe):
    osinfo = cijoe.getconf("os", None)
    if not osinfo:
        log.err("missing config value(os)")
        return errno.EINVAL

    xnvme_source = args.xnvme_source or cijoe.getconf("xnvme.repository.path", None)
    if not xnvme_source:
        return errno.EINVAL

    if osinfo["name"] == "windows":
        err, _ = cijoe.run(
            "powershell -c './toolbox/pkgs/windows-2022.ps1'", cwd=xnvme_source
        )
    else:
        err, _ = cijoe.run(
            f"./toolbox/pkgs/{osinfo['name']}-{osinfo['version']}.sh", cwd=xnvme_source
        )
    if err:
        return err

    return 0
