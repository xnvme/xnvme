#!/usr/bin/env python3
"""
Build liburing from source
==========================

Retargetable: True
------------------
"""
import errno
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument("--liburing_source", type=str, default=None)


def main(args, cijoe):
    """Build liburing"""

    liburing_source = args.liburing_source or cijoe.getconf(
        "liburing.repository.path", None
    )
    if not liburing_source:
        return errno.EINVAL

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
