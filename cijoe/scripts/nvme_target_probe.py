#!/usr/bin/env python3
"""
Probe a running NVMe TCP transport target
=========================================

Issue a discovery against a listener brought up by ``nvme_target_start`` and
print basic information about the exported namespace. Used as the check step
between ``nvme_target_start`` and ``nvme_target_stop`` in the transport demo
workflows.

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
        help="Target provider that brought the listener up",
    )
    parser.add_argument(
        "--traddr",
        type=str,
        default="127.0.0.1",
        help="Transport address (IP) of the listener",
    )
    parser.add_argument(
        "--trsvcid",
        type=str,
        default="4420",
        help="Transport service id (TCP port)",
    )
    parser.add_argument(
        "--trtype",
        type=str,
        default="tcp",
        help="Transport type",
    )


def _get_transport_device(cijoe):
    for device in cijoe.getconf("devices", []):
        if "fabrics" in device.get("labels", []):
            return device
    return None


def main(args, cijoe):
    """Discover and enumerate the listener brought up earlier."""

    device = _get_transport_device(cijoe)
    if not device:
        log.error("FAILED: no device labelled 'fabrics' in CIJOE config")
        return errno.ENOENT

    subnqn = device["subnqn"]

    uri = f"{args.traddr}:{args.trsvcid}"
    commands = [
        f"nvme discover -t {args.trtype} -a {args.traddr} -s {args.trsvcid}",
        f"xnvme enum --uri {uri}",
        f"xnvme info {uri} --subnqn {subnqn}",
        f"xnvmeperf run {uri} --subnqn {subnqn} --iopattern randread "
        f"--iosize 4096 --qdepth 32 --nqueues 1 --runtime 5 --cpumask 0x4",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd)
        if err:
            log.error("FAILED: %s (errno=%d)", cmd, err)
            return err

    return 0
