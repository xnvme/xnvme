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


def main(args, cijoe, step):
    conf = cijoe.config.options.get("xnvme", None)
    if not conf:
        return errno.EINVAL

    xnvme_source = step.get("with", {}).get("xnvme_source", conf["repository"]["path"])

    commands = [
        "cd builddir && meson compile",
        "cd builddir && meson install",
        "cd builddir && meson --internal uninstall",
        "cd builddir && meson install",
        "cat builddir/meson-logs/meson-log.txt",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source)
        if err:
            return err

    return 0
