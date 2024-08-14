#!/usr/bin/env python3
"""
clone and build fio
=======================================

Clone fio from fio.repository.remote into fio.repository.path, checkout
fio.repository.tag, and build it.

Retargetable: True
------------------
"""


def main(args, cijoe, step):
    """Build fio in the repository"""

    repos = cijoe.config.options.get("fio", {}).get("repository", {})
    if not repos:
        return 1

    commands = [
        (f"git clone {repos['remote']} {repos['path']} || true", "/tmp"),
        ("git clean -fdX", repos["path"]),
        (f"git checkout {repos['tag']} || true", repos["path"]),
        ("git rev-parse --short HEAD || true", repos["path"]),
        ("make", repos["path"]),
    ]

    for cmd, cwd in commands:
        err, _ = cijoe.run(cmd, cwd=cwd)
        if err:
            return err

    return err
