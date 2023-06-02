#!/usr/bin/env python3
"""
Build xNVMe bindings for Python
===============================

Step Args
---------

step.with.xnvme_source:  path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
from pathlib import Path


def main(args, cijoe, step):
    """Build xNVMe"""

    conf = cijoe.config.options.get("xnvme", None)
    if not conf:
        return errno.EINVAL

    xnvme_source = Path(
        step.get("with", {}).get("xnvme_source", conf["repository"]["path"])
    )

    commands = [
        "make uninstall",
        "make clean",
        "make build",
        "make install",
        "make test",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source / "python" / "bindings")
        if err:
            return err

    return 0
