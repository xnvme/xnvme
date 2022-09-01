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

Step Arguments
--------------

Requires a 'xnvme_source' pointing to the root of the xnvme source

    with:
      xnvme_source: '/tmp/xnvme_source'

Retargetable: True
------------------
"""
import errno
import logging as log
from pathlib import Path


def worklet_entry(args, cijoe, step):

    osinfo = cijoe.config.options.get("os", None)
    if not osinfo:
        log.err("cijoe.config.options is missing 'os'")
        return errno.EINVAL

    conf = cijoe.config.options.get("xnvme", None)
    if not conf:
        return errno.EINVAL

    xnvme_source = step.get("with", {}).get("xnvme_source", conf["repository"]["path"])

    err, _ = cijoe.run(
        f"./toolbox/pkgs/{osinfo['name']}-{osinfo['version']}.sh", cwd=xnvme_source
    )
    if err:
        return err

    return 0
