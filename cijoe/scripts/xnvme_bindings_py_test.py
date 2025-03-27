#!/usr/bin/env python3
"""
Test xNVMe bindings for Python
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
    """Test xNVMe"""

    xnvme_source = Path(
        step.get("with", {}).get(
            "xnvme_source", cijoe.getconf("xnvme.repository.path", None)
        )
    )
    if not xnvme_source:
        return errno.EINVAL

    err, _ = cijoe.run("pytest test_buf.py", cwd=xnvme_source / "python" / "tests")
    if err:
        return err

    return 0
