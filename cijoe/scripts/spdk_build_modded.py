#!/usr/bin/env python3
"""
Build SPDK from source
=======================

Step Args
---------

step.with.spdk_source:  path to SPDK source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
from pathlib import Path


def main(args, cijoe, step):
    """Build SPDK"""

    spdk_source = step.get("with", {}).get(
        "spdk_source", cijoe.getconf("spdk.repository.path", None)
    )
    if not spdk_source:
        return errno.EINVAL

    fio_repos = cijoe.getconf("fio.repository", None)
    spdk_prefix = cijoe.getconf("spdk.build.prefix", None)

    configure = " ".join(
        [
            "./configure",
            "--disable-unit-tests",
            "--enable-lto",
            f"--with-fio={fio_repos['path']}" if fio_repos else "",
            "--with-uring",
            "--with-xnvme",
            "--without-crypto",
            "--without-fuse",
            "--without-idxd",
            "--without-iscsi-initiator",
            "--without-nvme-cuse",
            "--without-ocf",
            "--without-rbd",
            "--without-usdt",
            "--without-vfio-user",
            "--without-vhost",
            "--without-virtio",
            "--without-vtune",
            f"--prefix={spdk_prefix}" if spdk_prefix else "",
        ]
    )
    commands = [
        "make clean || true",
        "git clean -dfx",
        "git rev-parse --short HEAD",
        "git status",
        "git submodule deinit -f .",
        "git status",
        "git submodule update --init --recursive",
        "git status",
        configure,
        "make -j $(nproc)",
        "make install",
    ]
    first_err = 0
    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=spdk_source)
        if err and not first_err:
            first_err = err

    return first_err
