#!/usr/bin/env python3
"""
Build liburing from source
==========================

Step Args
---------

Retargetable: True
------------------
"""
import errno
from pathlib import Path


def main(args, cijoe, step):
    """Build liburing"""

    conf = cijoe.config.options.get("liburing", None)
    if not conf:
        return errno.EINVAL

    liburing_source = step.get("with", {}).get(
        "liburing_source", conf["repository"]["path"]
    )

    commands = [
        "unlink /usr/lib/liburing.so || true",
        "unlink /usr/lib/liburing.so.2 || true",
        "rm /etc/ld.so.conf.d/spdk-liburing.conf || true",
        "rm -r /usr/include/liburing/ || true",
        "rm /usr/include/liburing.h || true",
        "rm /usr/include/liburing/barrier.h || true",
        "rm /usr/include/liburing/compat.h || true",
        "rm /usr/include/liburing/io_uring.h || true",
        "rm /usr/lib/liburing.a || true",
        "rm /usr/lib/liburing.so.2.2 || true",
        "rm /usr/lib/pkgconfig/liburing.pc || true",
    ]

    commands += [
        "git rev-parse --short HEAD",
        "make clean",
        "./configure",
        "make -j $(nproc)",
        "make install",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=liburing_source)
        if err and not first_err:
            first_err = err

    return first_err
