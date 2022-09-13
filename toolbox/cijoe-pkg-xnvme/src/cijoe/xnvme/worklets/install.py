#!/usr/bin/env python3
"""
Install xNVMe from previously built source
==========================================

Step Args
---------

step.with.xnvme_source:  path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
from pathlib import Path


def worklet_entry(args, cijoe, step):

    conf = cijoe.config.options.get("xnvme", None)
    if not conf:
        return errno.EINVAL

    xnvme_source = step.get("with", {}).get("xnvme_source", conf["repository"]["path"])

    commands = [
        "meson install -C builddir",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source)
        if err:
            return err

    return 0
