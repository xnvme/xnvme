#!/usr/bin/env python3
"""
Build SPDK and the SPDK FIO Plugin from Source
==============================================

This process enables latency comparison using a consistent tool: fio.

To achieve this, build SPDK without xNVMe support but with the fio plugin enabled.
This configuration allows using the SPDK NVMe driver via the SPDK fio plugin.

This does not install SPDK, since it is not needed to make use.

Retargetable: True
------------------
"""
import errno
import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--spdk_source", type=str, default=None, help="path to SPDK source"
    )


def main(args, cijoe):
    """Build SPDK"""

    spdk_source = args.spdk_source or cijoe.getconf("spdk.repository.path", None)
    if not spdk_source:
        return errno.EINVAL

    fio_repos = cijoe.getconf("fio.repository", None)

    configure = " ".join(
        [
            "./configure",
            "--disable-unit-tests",
            "--disable-tests",
            "--enable-lto",
            f"--with-fio={fio_repos['path']}" if fio_repos else "",
            "--without-crypto",
            "--without-fuse",
            "--without-idxd",
            "--without-iscsi-initiator",
            "--without-nvme-cuse",
            "--without-ocf",
            "--without-rbd",
            "--without-uring",
            "--without-usdt",
            "--without-vfio-user",
            "--without-vhost",
            "--without-virtio",
            "--without-vtune",
            "--without-xnvme",
        ]
    )
    commands = [
        "gmake clean || true",
        "git clean -dfx || true",
        "git rev-parse --short HEAD || true",
        "git status || true",
        "git submodule deinit -f . || true",
        "git status || true",
        "git submodule update --init --recursive || true",
        "git status || true",
        configure,
        "gmake -j $(nproc)",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=spdk_source)
        if err and not first_err:
            log.error(f"cmd({cmd}); err({err})")
            first_err = err

    return first_err
