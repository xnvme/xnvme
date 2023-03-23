#!/usr/bin/env python3
"""
Start a qemu-guest with NVMe devices
====================================

This creates a configuration, which on a recent Linux installation would be
something like:

* /dev/ng0n1 -- nvm
* /dev/ng0n2 -- zns
* /dev/ng0n3 -- kv

* /dev/ng1n1 -- nvm

* /dev/ng2n1 -- nvm (mdts=0 / unlimited)

Using the the 'ng' device-handle, as it is always available, whereas 'nvme' are
only for block-devices. Regardless, the above is just to illustrate one
possible "appearance" of the devices in Linux.

Retargetable: false
-------------------
"""
import errno
import logging as log
from pathlib import Path

from cijoe.qemu.wrapper import Guest


def worklet_entry(args, cijoe, step):
    """Start a qemu guest"""

    guest = Guest(cijoe, cijoe.config)

    nvme_img_root = Path(step.get("with", {}).get("nvme_img_root", guest.guest_path))

    # NVMe configuration arguments, a single controller with two namespaces
    lbads = 12
    drive_size = "8G"
    drives = []

    # pcie setup
    nvme = []
    nvme += ["-device pcie-root-port,id=pcie_root_port1,chassis=1,slot=1"]
    upstream_bus = "pcie_upstream_port1"
    nvme += [f"-device x3130-upstream,id={upstream_bus},bus=pcie_root_port1"]

    def gen_controller(id, serial, mdts, downstream_bus, upstream_bus, controller_slot):
        controller = {
            "id": id,
            "serial": serial,
            "bus": downstream_bus,
            "mdts": mdts,
            "ioeventfd": "on",
        }

        return [
            "-device xio3130-downstream"
            f",id={downstream_bus},bus={upstream_bus},chassis=2,slot={controller_slot}",
            "-device nvme," + ",".join(f"{k}={v}" for k, v in controller.items()),
        ]

    def gen_namespace(controller_id, nsid, additional_attributes={}):
        drive_id = f"{controller_id}n{nsid}"
        drive = {
            "id": drive_id,
            "file": str(nvme_img_root / f"{drive_id}.img"),
            "format": "raw",
            "if": "none",
            "discard": "on",
            "detect-zeroes": "unmap",
        }
        # drives.append(drive1)
        controller_namespace = {
            "id": drive_id,
            "drive": drive_id,
            "bus": controller_id,
            "nsid": nsid,
            "logical_block_size": 1 << lbads,
            "physical_block_size": 1 << lbads,
            **additional_attributes,
        }
        return drive, [
            "-drive " + ",".join(f"{k}={v}" for k, v in drive.items()),
            "-device nvme-ns,"
            + ",".join(f"{k}={v}" for k, v in controller_namespace.items()),
        ]

    # Nvme0 - Standard NVMe setup. Conventional and zoned namespaces.
    controller_id1 = "nvme0"
    controller_bus1 = "pcie_downstream_port1"
    controller_slot1 = 1
    nvme += gen_controller(
        controller_id1, "deadbeef", 7, controller_bus1, upstream_bus, controller_slot1
    )

    # Nvme0n1 - Conventional namespace
    drive1, qemu_nvme_dev1 = gen_namespace(controller_id1, 1)
    nvme += qemu_nvme_dev1
    drives.append(drive1)

    # Nvme0n2 - Zoned namespace
    zoned_attributes = {
        "zoned": "on",
        "zoned.zone_size": "32M",
        "zoned.zone_capacity": "28M",
        "zoned.max_active": 256,
        "zoned.max_open": 256,
        "zoned.zrwas": 32 << lbads,
        "zoned.zrwafg": 16 << lbads,
        "zoned.numzrwa": 256,
    }

    drive2, qemu_nvme_dev2 = gen_namespace(controller_id1, 2, zoned_attributes)
    nvme += qemu_nvme_dev2
    drives.append(drive2)

    # Nvme0n3 - KV namespace
    kv_attributes = {
        "kv": "on",
    }

    drv_kv, qemu_nvme_dev_kv = gen_namespace(controller_id1, 3, kv_attributes)
    nvme += qemu_nvme_dev_kv
    drives.append(drv_kv)

    # Nvme1 - Fabrics NVMe setup
    controller_id2 = "nvme1"
    controller_bus2 = "pcie_downstream_port2"
    controller_slot2 = 2
    nvme += gen_controller(
        controller_id2, "adcdbeef", 7, controller_bus2, upstream_bus, controller_slot2
    )

    # Nvme1n1 - Conventional namespace
    drive3, qemu_nvme_dev3 = gen_namespace(controller_id2, 1)
    nvme += qemu_nvme_dev3
    drives.append(drive3)

    # Nvme2 - Large MDTS NVMe setup
    controller_id3 = "nvme2"
    controller_bus3 = "pcie_downstream_port3"
    controller_slot3 = 3
    nvme += gen_controller(
        controller_id3, "beefcace", 0, controller_bus3, upstream_bus, controller_slot3
    )

    # Nvme2n1 - Conventional namespace
    drive4, qemu_nvme_dev4 = gen_namespace(controller_id3, 1)
    nvme += qemu_nvme_dev4
    drives.append(drive4)

    # Check that the backing-storage exists, create them if they do not
    for drive in drives:
        err, _ = cijoe.run_local(f"[ -f { drive['file'] } ]")
        if err:
            guest.image_create(drive["file"], drive["format"], drive_size)
        err, _ = cijoe.run_local(f"[ -f { drive['file'] } ]")

    err = guest.start(extra_args=nvme)
    if err:
        log.error(f"guest.start() : err({err})")
        return err

    started = guest.is_up()
    if not started:
        log.error("guest.is_up() : False")
        return errno.EAGAIN

    return 0
