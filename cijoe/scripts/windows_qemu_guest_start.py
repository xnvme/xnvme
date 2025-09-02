#!/usr/bin/env python3
"""
Start a qemu guest
==================

Starts the qemu guest with the given guest name. Fails if the guest is not up
within 180 seconds.

Retargetable: False
-------------------
"""
import errno
import logging as log
from argparse import ArgumentParser

from cijoe.qemu.wrapper import Guest


def add_args(parser: ArgumentParser):
    parser.add_argument("--guest_name", type=str, help="Name of the qemu guest.")


def main(args, cijoe):
    """Start a qemu Windows guest"""

    if "windows" != cijoe.getconf("os.name"):
        log.info("Not a Windows image; skipping")
        return 0

    if "guest_name" not in args:
        log.error("missing argument: guest_name")
        return 1

    guest = Guest(cijoe, cijoe.config, args.guest_name)

    err = guest.start()
    if err:
        log.error(f"guest.start() : err({err})")
        return err

    started = guest.is_up(timeout=60)
    err, _ = cijoe.run("hostname")
    if err and not started:
        log.error("guest.is_up() : False")
        return errno.EAGAIN

    return 0
