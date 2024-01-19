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

    repos = cijoe.config.options.get("fio", {}).get("repository", {})
    if not repos:
        return 1

    err, _ = cijoe.run("gmake --version")
    make = "make" if err else "gmake"

    commands = [
        (f"git clone {repos['remote']} {repos['path']} || true", "/tmp"),
        (f"git checkout {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
        (f"{make} clean", repos["path"]),
    ]

    prefix = cijoe.config.options.get("fio", {}).get("build", {}).get("prefix", {})
    if prefix:
        commands += [
            (f"./configure --prefix={prefix}", repos["path"]),
        ]

    commands += [
        (f"{make}", repos["path"]),
        (f"{make} install", repos["path"]),
    ]

    for cmd, cwd in commands:
        err, _ = cijoe.run(cmd, cwd=cwd)
        if err:
            return err

    return err
