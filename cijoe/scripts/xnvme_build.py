#!/usr/bin/env python3
"""
Build xNVMe from source
=======================

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

    xnvme_source = step.get("with", {}).get(
        "xnvme_source", conf.get("repository", {}).get("path", None)
    )
    if not xnvme_source:
        return errno.EINVAL

    os_name = cijoe.config.options.get("os", {}).get("name", "")

    commands = [
        "git rev-parse --short HEAD || true",
        "rm -r builddir || true",
        (
            'meson setup builddir --prefix "C:/tools/msys64/usr" --default-library=static'
            if os_name == "windows"
            else "meson setup builddir"
        ),
        "meson compile -C builddir",
        "cat builddir/meson-logs/meson-log.txt",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=xnvme_source)
        if err and not first_err:
            first_err = err

    return first_err
