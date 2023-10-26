#!/usr/bin/env python3
"""
Create a SPDK bdev configuration files
======================================

Creates configurations for the bdev-implementations

* bdev_nvme
* bdev_aio
* bdev_uring
* bdev_xnvme
  - io_mechanisms=[libaio, io_uring, io_uring_cmd]
  - conserve_cpu=[0, 1]

These are produced in sets with 1...n-1 duts.

The configuration-files are named:

* bdev_{BDEV_NAME}_{IO-INTERFACE}_[conserve_cpu]_{NDEV}.conf

The files are created locally in:

    cijoe-output/artifacts/bdev_confs/*.conf

and then transferred to:

    step.args.bdev_confs

By default this is "/tmp/bdev_confs"
"""
import argparse
import copy
import json
import logging as log
import os
import pprint
from pathlib import Path

from cijoe.core.resources import dict_from_yamlfile

IOPATHS = {
    "driver": {
        "bdev_name": "bdev_nvme",
        "io_mechanism": "spdk_nvme",
        "method": "bdev_nvme_attach_controller",
    },
    "aio": {
        "bdev_name": "bdev_aio",
        "io_mechanism": "libaio",
        "method": "bdev_aio_create",
    },
    "uring": {
        "bdev_name": "bdev_uring",
        "io_mechanism": "io_uring",
        "method": "bdev_uring_create",
    },
    "xnvme_libaio": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "libaio",
        "conserve_cpu": False,
        "method": "bdev_xnvme_create",
    },
    "xnvme_libaio_conserve_cpu": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "libaio",
        "conserve_cpu": True,
        "method": "bdev_xnvme_create",
    },
    "xnvme_io_uring": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "io_uring",
        "conserve_cpu": False,
        "method": "bdev_xnvme_create",
    },
    "xnvme_io_uring_conserve_cpu": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "io_uring",
        "conserve_cpu": True,
        "method": "bdev_xnvme_create",
    },
    "xnvme_io_uring_cmd": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "io_uring_cmd",
        "conserve_cpu": False,
        "method": "bdev_xnvme_create",
    },
    "xnvme_io_uring_cmd_conserve_cpu": {
        "bdev_name": "bdev_xnvme",
        "io_mechanism": "io_uring_cmd",
        "conserve_cpu": True,
        "method": "bdev_xnvme_create",
    },
}


def main(args, cijoe, step):
    duts = copy.deepcopy(cijoe.config.options.get("duts", []))
    if not duts:
        log.error("Missing 'duts' section in CIJOE configuration file")
        return 1

    bdev_confs = step.get("with", {}).get("bdev_confs", "/tmp")

    output_dir = cijoe.output_path / "artifacts" / "bdev_confs"

    os.makedirs(output_dir, exist_ok=True)

    for iopath_label, iopath in IOPATHS.items():
        conf = {"subsystems": []}
        for count, dev_info in enumerate(duts, 1):
            bdevs = {
                "subsystem": "bdev",
                "config": [],
            }

            # Parameters, first the device "handle"
            params = {}
            if "driver" in iopath_label:
                params["trtype"] = "PCIe"
                params["traddr"] = f"{dev_info['pcie']}"
            elif "io_uring_cmd" in iopath_label:
                params["filename"] = f"/dev/ng{dev_info['os']}"
            else:
                params["filename"] = f"/dev/nvme{dev_info['os']}"

            # Parameters, then an instance name
            params["name"] = "_".join(
                [
                    f"{iopath['bdev_name']}",
                    f"{iopath['io_mechanism']}",
                    f"device{dev_info['os']}",
                ]
            )

            # Parameters, for xNVMe
            if "xnvme" in iopath_label:
                params["io_mechanism"] = f"{iopath['io_mechanism']}"
                params["conserve_cpu"] = iopath["conserve_cpu"]

            bdevs["config"].append(
                {
                    "params": params,
                    "method": iopath["method"],
                }
            )
            conf["subsystems"].append(bdevs)

            filename = output_dir / "_".join(
                [
                    f"{iopath['bdev_name']}",
                    f"{iopath['io_mechanism']}",
                    "conserve_cpu"
                    if "xnvme" in iopath_label and iopath["conserve_cpu"]
                    else "",
                    f"{count}.conf",
                ]
            )

            with filename.open("w") as cfile:
                json.dump(conf, cfile, indent=2, sort_keys=False)

    cijoe.run(f"ls {bdev_confs} || true")
    cijoe.run(f"rm -r {bdev_confs} || true")
    cijoe.put(output_dir, f"{bdev_confs}")
    cijoe.run(f"ls {bdev_confs}")


if __name__ == "__main__":
    main()
