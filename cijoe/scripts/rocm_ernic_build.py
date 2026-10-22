#!/usr/bin/env python3
"""
Build the rocm-ernic vfio-user server
=====================================

Builds libvfio-user and the rocm-ernic server at the revisions pinned in
``[rocm_ernic]`` and ``[rocm_ernic.libvfio_user]``, installing both under
``rocm_ernic.prefix``, so neither needs root or touches /usr.

The server has to be built from the same rocm-ernic revision as the guest's
patched ionic driver; a mismatch does not fail cleanly but shows up as a
wedged device (DSR -ETIMEDOUT, "Failed to register ibdev") in the guest.

The build is skipped when the prefix already holds a server built from the
pinned revisions; a stamp next to the binary records them.

Missing build dependencies are installed with apt-get when running as root,
as in a CI container, and otherwise reported for the operator to install.

Retargetable: False
-------------------
"""

import errno
import logging as log
import os
from argparse import ArgumentParser
from pathlib import Path

PKGCONFIG_DEPS = ["libibverbs", "librdmacm", "glib-2.0", "json-c", "cmocka"]
APT_DEPS = [
    "cmake",
    "meson",
    "ninja-build",
    "pkg-config",
    "gcc",
    "git",
    "curl",
    "libibverbs-dev",
    "librdmacm-dev",
    "libglib2.0-dev",
    "libjson-c-dev",
    "libcmocka-dev",
]


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--force", action="store_true", help="Rebuild even when the stamp matches."
    )


def ensure_deps(cijoe):
    missing = [
        dep
        for dep in PKGCONFIG_DEPS
        if cijoe.run_local(f"pkg-config --exists {dep}")[0]
    ]
    if not missing:
        return 0

    if os.geteuid() != 0 or cijoe.run_local("command -v apt-get")[0]:
        log.error(f"missing build dependencies: {' '.join(missing)}")
        log.error(f"on Debian/Ubuntu: sudo apt-get install {' '.join(APT_DEPS)}")
        return errno.ENOENT

    err, _ = cijoe.run_local(
        "DEBIAN_FRONTEND=noninteractive apt-get -qy update && "
        f"DEBIAN_FRONTEND=noninteractive apt-get -qy install {' '.join(APT_DEPS)}"
    )
    return err


def build(cijoe, force=False):
    """Build libvfio-user and the server under the prefix; return the binary path
    on success, None otherwise"""

    prefix = Path(cijoe.getconf("rocm_ernic.prefix"))
    ernic_repo = cijoe.getconf("rocm_ernic.repository")
    ernic_sha = cijoe.getconf("rocm_ernic.sha")
    vfu_repo = cijoe.getconf("rocm_ernic.libvfio_user.repository")
    vfu_sha = cijoe.getconf("rocm_ernic.libvfio_user.sha")

    binary = prefix / "bin" / "rocm-ernic"
    stamp = prefix / "bin" / "rocm-ernic.stamp"
    want = f"rocm-ernic {ernic_sha} libvfio-user {vfu_sha}"
    if not force and binary.exists() and stamp.exists():
        if stamp.read_text().strip() == want:
            log.info(f"{binary} already built from: {want}")
            return binary

    if ensure_deps(cijoe):
        return None

    src = prefix / "src"
    vfu_src = src / "libvfio-user"
    ernic_src = src / "rocm-ernic"

    # The libvfio-user pin is an abbreviated SHA, which a shallow fetch cannot
    # resolve, so this is a full clone.
    if cijoe.run_local(
        f"rm -rf {vfu_src} && mkdir -p {src} && "
        f"git clone -q {vfu_repo} {vfu_src} && "
        f"git -C {vfu_src} checkout -q {vfu_sha} && "
        f"cd {vfu_src} && "
        f"meson setup build --prefix={prefix} --libdir=lib -Dwarning_level=0 "
        # A debug build adds -Werror, and its unit tests use cmocka macros
        # that newer cmocka deprecates.
        "--buildtype=release && "
        "ninja -C build && ninja -C build install"
    )[0]:
        log.error("libvfio-user build failed")
        return None

    if cijoe.run_local(
        f"rm -rf {ernic_src} && mkdir -p {ernic_src} && "
        f"curl -fsSL {ernic_repo}/archive/{ernic_sha}.tar.gz "
        f"| tar -xz --strip-components=1 -C {ernic_src} && "
        f"cd {ernic_src} && "
        f"PKG_CONFIG_PATH={prefix}/lib/pkgconfig "
        "cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "
        f"-DCMAKE_PREFIX_PATH={prefix} -DCMAKE_INSTALL_RPATH={prefix}/lib && "
        "cmake --build build && "
        f"install -D -m 0755 build/rocm-ernic {binary}"
    )[0]:
        log.error("rocm-ernic build failed")
        return None

    stamp.write_text(want + "\n")
    return binary


def main(args, cijoe):
    """Build libvfio-user and the rocm-ernic server under the prefix"""

    return 0 if build(cijoe, args.force) else errno.EIO
