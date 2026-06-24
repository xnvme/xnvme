#!/usr/bin/env python3
"""
Tear down an NVMe TCP transport target
======================================

Tear down whichever NVMe TCP target was set up by ``nvme_target_start``.
Two providers are supported via ``--provider``:

* ``spdk`` (default): stop the SPDK ``nvmf_tgt`` process.
* ``linux``: remove the Linux kernel ``nvmet`` configfs entries.

Both providers are safe to invoke when the target is already down.

Retargetable: True
"""
import errno
import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--provider",
        choices=["spdk", "linux"],
        default="spdk",
        help="Target provider: SPDK nvmf_tgt or Linux kernel nvmet",
    )


def _get_transport_device(cijoe):
    """Return the first device labelled for NVMe transport export."""

    for device in cijoe.getconf("devices", []):
        if "fabrics" in device.get("labels", []):
            return device
    return None


def _stop_spdk(args, cijoe):
    """Stop the SPDK ``nvmf_tgt`` process."""

    cijoe.run("pgrep -f nvmf_tgt && pkill -f nvmf_tgt; true")
    log.info("spdk target down")
    return 0


def _stop_linux(args, cijoe):
    """Remove the Linux kernel ``nvmet`` configfs entries."""

    device = _get_transport_device(cijoe)
    if not device:
        return 0

    subnqn = device["subnqn"]
    nvmet = "/sys/kernel/config/nvmet"

    for cmd in (
        f"rm -f {nvmet}/ports/1/subsystems/{subnqn}",
        f"rmdir {nvmet}/ports/1; true",
        f"echo 0 > {nvmet}/subsystems/{subnqn}/namespaces/1/enable; true",
        f"rmdir {nvmet}/subsystems/{subnqn}/namespaces/1; true",
        f"rmdir {nvmet}/subsystems/{subnqn}; true",
    ):
        cijoe.run(cmd)

    log.info("linux target down")
    return 0


def main(args, cijoe):
    """Tear down an NVMe TCP target using the selected provider."""

    if args.provider == "spdk":
        return _stop_spdk(args, cijoe)
    if args.provider == "linux":
        return _stop_linux(args, cijoe)
    log.error("unknown provider: %s", args.provider)
    return errno.EINVAL
