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

    xnvme_source = Path(
        step.get("with", {}).get(
            "xnvme_source", cijoe.getconf("xnvme.repository.path", None)
        )
    )
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
        err, _ = cijoe.run(cmd, cwd=xnvme_source / "python" / "bindings")
        if err:
            return err

    return 0
