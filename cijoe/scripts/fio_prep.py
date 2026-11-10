#!/usr/bin/env python3
"""
clone, configure, build and install fio
=======================================

Clone fio from fio.repository.remote into fio.repository.path, checkout
fio.repository.tag, apply the patches xNVMe carries for fio in
toolbox/fio/patches, configure using fio.build.prefix when available, build and
install it.

Retargetable: True
------------------
"""

from pathlib import Path

PATCHES = Path(__file__).resolve().parents[2] / "toolbox" / "fio" / "patches"


def apply_patches(cijoe, repos):
    """Send each patch into the fio checkout and apply it, skipping one already applied"""

    for patch in sorted(PATCHES.glob("fio-0*.patch")):
        if not cijoe.put(str(patch), f"{repos['path']}/{patch.name}"):
            return 1
        err, _ = cijoe.run(
            f"git apply -R --check {patch.name} 2>/dev/null || git apply {patch.name}",
            cwd=repos["path"],
        )
        if err:
            return err

    return 0


def main(args, cijoe):
    """Configure, build and install fio in 'config.options.fio.build.prefix'"""

    repos = cijoe.getconf("fio.repository", {})
    if not repos:
        return 1

    os_name = cijoe.getconf("os.name", "")

    if os_name == "windows":
        commands = fio_commands_windows(repos)
    else:
        commands = fio_commands(cijoe, repos)

    for cmd, cwd in commands:
        if cmd is None:
            err = apply_patches(cijoe, repos)
        else:
            err, _ = cijoe.run(cmd, cwd=cwd)
        if err:
            return err

    return err


def fio_commands(cijoe, repos):
    shell = cijoe.getconf("cijoe.run.shell", "sh")
    os_name = cijoe.getconf("os.name", "")

    make = "gmake" if shell == "csh" or os_name == "macos" else "make"

    commands = [
        (f"git clone {repos['remote']} {repos['path']} || true", "/tmp"),
        (f"git checkout -f {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
        (None, None),
        (f"{make} clean", repos["path"]),
    ]

    prefix = cijoe.getconf("fio.build.prefix", {})
    if prefix:
        commands += [
            (f"./configure --prefix={prefix}", repos["path"]),
        ]

    commands += [
        (f"{make}", repos["path"]),
    ]

    if os_name != "macos":
        commands += [
            (f"{make} install", repos["path"]),
        ]

    return commands


def fio_commands_windows(repos):
    commands = [
        (f"git clone {repos['remote']} {repos['path']} || true", "/tmp"),
        ("git clean -fdX", repos["path"]),
        (f"git checkout -f {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
        (None, None),
        ("make CFLAGS='-O0'", repos["path"]),
    ]

    return commands
