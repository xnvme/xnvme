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


def main(args, cijoe):
    """Build and install MacVFN"""

    username = cijoe.getconf("cijoe.transport.nonroot", None)
    if not username:
        log.error(
            "Missing non-root user transport from config. MacVFN must be installed without root access."
        )

    macvfn_source = cijoe.getconf("macvfn.repository.path", None)
    if not macvfn_source:
        log.error("Missing path to MacVFN source.")
        return errno.EINVAL

    os_name = cijoe.getconf("os.name", "")
    if os_name != "macos":
        log.error(f"OS requirement: MacOS - was {os_name}")
        return errno.EINVAL

    commands = ["make kill", "make build", "make install"]

    err = 0

    for cmd in commands:
        err, _ = cijoe.run(cmd, cwd=macvfn_source, transport_name="nonroot")
        if err:
            log.error(f"error({cmd})")
            return err

    return err
