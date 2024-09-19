#!/usr/bin/env python3
"""
Build and install MacVFN
========================

Prerequisites
-------------
* OS: MacOS
* System Integrity Protection disabled

Retargetable: True
------------------
"""
import errno
import logging as log


def main(args, cijoe, step):
    """Build and install MacVFN"""

    conf = cijoe.config.options.get("macvfn", {})
    macvfn_source = conf.get("repository", {}).get("path", None)
    if not macvfn_source:
        log.error("Missing path to MacVFN source.")
        return errno.EINVAL

    os_name = cijoe.config.options.get("os", {}).get("name", "")
    if os_name != "macos":
        log.error(f"OS requirement: MacOS - was {os_name}")
        return errno.EINVAL

    err, _ = cijoe.run("make build install kill", cwd=macvfn_source)

    return err
