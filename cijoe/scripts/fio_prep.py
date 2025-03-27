#!/usr/bin/env python3
"""
clone, configure, build and install fio
=======================================

Clone fio from fio.repository.remote into fio.repository.path, checkout
fio.repository.tag, configure using fio.build.prefix when available, build and
install it.

Retargetable: True
------------------
"""


def main(args, cijoe, step):
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
        (f"git checkout {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
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
        (f"git checkout {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
        ("make", repos["path"]),
    ]

    return commands
