#!/usr/bin/env python3
"""
Unlock root SSH on a nosi guest
===============================

nosi images ship safe-by-default: the root account is LOCKED; you log in as the
operator 'odus' (passwordless sudo, SSH password auth on first boot). xNVMe's
cijoe workflows assume a root SSH transport, so this step -- run over the 'odus'
transport -- sets a root password and enables 'PermitRootLogin', then waits for
the root transport to come up. After it runs, the rest of the workflow uses the
default (root) transport unchanged.

Config: requires an operator transport (default name 'odus') and the default
transport to be root. The sshd drop-in is named to sort before any hardening
drop-ins so its 'PermitRootLogin yes' wins.

Retargetable: True (uses the 'odus' and the default transports).
"""
import logging as log
import time
from argparse import ArgumentParser

ROOT_PASSWORD = "root"
SSHD_DROPIN = "/etc/ssh/sshd_config.d/00-ci-root.conf"


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--operator",
        type=str,
        default="odus",
        help="Transport name of the passwordless-sudo operator account.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Seconds to wait for the operator and the root transport.",
    )


def wait_for_transport(cijoe, transport_name, timeout):
    """Return True once 'transport_name' accepts SSH within 'timeout' seconds."""

    began = time.time()
    while time.time() - began < timeout:
        err, _ = cijoe.run("true", transport_name=transport_name)
        if not err:
            return True
        time.sleep(5)
    return False


def main(args, cijoe):
    """Open root SSH over the operator account, then wait for root to be up."""

    # No-op when no operator transport is configured (e.g. the freebsd guest or
    # the fork-based guest where root SSH already works).
    transports = cijoe.config.options.get("cijoe", {}).get("transport", {})
    if args.operator not in transports:
        log.info(f"no '{args.operator}' transport configured; skipping root unlock")
        return 0

    if not wait_for_transport(cijoe, args.operator, args.timeout):
        log.error(f"operator transport '{args.operator}' did not come up")
        return 1

    commands = [
        f"echo 'root:{ROOT_PASSWORD}' | sudo chpasswd",
        f"echo 'PermitRootLogin yes' | sudo tee {SSHD_DROPIN}",
        f"echo 'PasswordAuthentication yes' | sudo tee -a {SSHD_DROPIN}",
        "sudo systemctl restart ssh || sudo systemctl restart sshd",
    ]
    for command in commands:
        err, _ = cijoe.run(command, transport_name=args.operator)
        if err:
            log.error(f"root-unlock command failed: {command}")
            return err

    if not wait_for_transport(cijoe, None, args.timeout):
        log.error("root transport did not come up after unlock")
        return 1

    log.info("root SSH unlocked")
    return 0
