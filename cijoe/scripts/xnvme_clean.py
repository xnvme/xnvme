#!/usr/bin/env python3
"""
Clean xNVMe from source / repository
====================================

Step Args
---------

step.with.xnvme_source: path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
from pathlib import Path


def main(args, cijoe, step):
    """Clean xNVMe"""

    xnvme_source = step.get("with", {}).get(
        "xnvme_source", cijoe.getconf("xnvme.repository.path", None)
    )
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
