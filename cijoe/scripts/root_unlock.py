#!/usr/bin/env python3
"""
Unlock root SSH on a nosi guest
===============================

nosi images ship with root locked; log in is via the 'odus' operator (passwordless
sudo). This step opens root SSH over that operator so the rest of the task can
use the default root target unchanged.

Retargetable: True (uses the 'root' and 'odus' targets).
"""
import logging as log
import time
from argparse import ArgumentParser

ROOT_PASSWORD = "root"
SSHD_DROPIN = "/etc/ssh/sshd_config.d/00-ci-root.conf"


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--root",
        type=str,
        default="root",
        help="Target name of the root account.",
    )
    parser.add_argument(
        "--operator",
        type=str,
        default="odus",
        help="Target name of the passwordless-sudo operator account.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Seconds to wait for the operator and the root target.",
    )
    parser.add_argument(
        "--root-probe-timeout",
        type=int,
        default=30,
        help="Seconds to probe root SSH before deciding to unlock.",
    )


def wait_for_target(cijoe, target, timeout):
    """Return True once 'target' accepts SSH within 'timeout' seconds."""

    began = time.time()
    while time.time() - began < timeout:
        err, _ = cijoe.run("true", target=target)
        if not err:
            return True
        time.sleep(5)
    return False


def detect_os(cijoe, target):
    """Return 'linux', 'freebsd', or '' over the operator target."""

    err, state = cijoe.run("uname -s", target=target)
    if err:
        return ""
    try:
        output = state.output()
    except (AttributeError, OSError):
        return ""
    name = output.strip().lower()
    if "freebsd" in name:
        return "freebsd"
    if "linux" in name:
        return "linux"
    return ""


def unlock_commands(os_name):
    """Return the shell commands to unlock root for the given uname -s family.

    Each list is executed in order on the operator target. A failure
    in any single command should abort the unlock and return the error.
    """

    if os_name == "freebsd":
        # `pw unlock` clears the *LOCKED* prefix; `pw usermod -h 0` reads the
        # new password from stdin. Some pw versions don't combine both steps.
        return [
            "sudo pw unlock root",
            f"echo '{ROOT_PASSWORD}' | sudo pw usermod root -h 0",
            f"echo 'PermitRootLogin yes' | sudo tee {SSHD_DROPIN}",
            f"echo 'PasswordAuthentication yes' | sudo tee -a {SSHD_DROPIN}",
            "sudo service sshd reload",
        ]
    # ssh unit name varies: Debian/Ubuntu = ssh, Fedora = sshd.
    return [
        f"echo 'root:{ROOT_PASSWORD}' | sudo chpasswd",
        f"echo 'PermitRootLogin yes' | sudo tee {SSHD_DROPIN}",
        f"echo 'PasswordAuthentication yes' | sudo tee -a {SSHD_DROPIN}",
        "sudo systemctl restart ssh || sudo systemctl restart sshd",
    ]


def main(args, cijoe):
    """Open root SSH over the operator account, then wait for root to be up."""

    # No-op when root SSH already works (fork-based or pre-unlocked images).
    if wait_for_target(cijoe, args.root, args.root_probe_timeout):
        log.info("root SSH already available; skipping unlock")
        return 0

    if args.operator not in cijoe.targets:
        log.error(
            f"root SSH not available and no '{args.operator}' target to unlock with"
        )
        return 1

    if not wait_for_target(cijoe, args.operator, args.timeout):
        log.error(f"operator target '{args.operator}' did not come up")
        return 1

    os_name = detect_os(cijoe, args.operator)
    if not os_name:
        log.error("could not detect guest OS (uname -s) over operator target")
        return 1
    log.info(f"unlocking root on {os_name} guest")

    for command in unlock_commands(os_name):
        err, _ = cijoe.run(command, target=args.operator)
        if err:
            log.error(f"root-unlock command failed: {command}")
            return err

    if not wait_for_target(cijoe, args.root, args.timeout):
        log.error("root target did not come up after unlock")
        return 1

    log.info("root SSH unlocked")
    return 0
