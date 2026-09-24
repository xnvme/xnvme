#!/usr/bin/env python3
"""
Probe a running NVMe transport target
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
        "--nvme-trtype",
        type=str,
        default="tcp",
        choices=["tcp", "rdma"],
        help="Transport type for the NVMe listener",
    )
    parser.add_argument(
        "--transport-name",
        type=str,
        default=None,
        help="Transport to use for cijoe.run() commands",
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

    uri = device["uri"]
    traddr, trsvcid = uri.rsplit(":", 1)
    commands = [
        f"nvme discover -t {args.nvme_trtype} -a {traddr} -s {trsvcid}",
        f"xnvme enum --uri {uri}",
        f"xnvme info {uri} --subnqn {subnqn}",
        f"xnvmeperf run {uri} --subnqn {subnqn} --iopattern randread "
        f"--iosize 4096 --qdepth 32 --nqueues 1 --runtime 5 --cpumask 0x4",
    ]
    for cmd in commands:
        err, _ = cijoe.run(cmd, transport_name=args.transport_name)
        if err:
            log.error("FAILED: %s (errno=%d)", cmd, err)
            return err

    return 0
