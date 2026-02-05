#!/usr/bin/env python3
"""
Start a qemu-guest with NVMe devices
====================================

This creates a configuration with one namespace per controller to avoid
Linux kernel namespace ordering issues. On a recent Linux installation:

* /dev/ng0n1 -- nvm
* /dev/ng1n1 -- zns
* /dev/ng2n1 -- kv
* /dev/ng3n1 -- nvm (fabrics testing)
* /dev/ng4n1 -- nvm (large mdts)
* /dev/ng5n1 -- fdp
* /dev/ng6n1 -- pi type 1
* /dev/ng7n1 -- pi type 2
* /dev/ng8n1 -- pi type 3
* /dev/ng9n1 -- pi type 1, extended LBA
* /dev/ng10n1 -- pi type 1, pif 2

Using the 'ng' device-handle, as it is always available, whereas 'nvme' are
only for block-devices.

Retargetable: false
-------------------
"""
import errno
import logging as log
import subprocess
from argparse import ArgumentParser
from pathlib import Path

from cijoe.qemu.wrapper import Guest


def detect_qemu_nvme_features(qemu_bin):
    """
    Detect which NVMe features the QEMU binary supports by parsing device help.

    Returns dict with feature flags: {kv, zns, fdp, pi}
    """
    features = {"kv": False, "zns": False, "fdp": False, "pi": False}

    try:
        # Check nvme-ns device properties
        result = subprocess.run(
            [qemu_bin, "-device", "nvme-ns,help"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        ns_help = result.stdout + result.stderr
        features["kv"] = "kv=" in ns_help
        features["zns"] = "zoned=" in ns_help
        features["pi"] = "pi=" in ns_help

        # Check nvme-subsys device properties for FDP
        result = subprocess.run(
            [qemu_bin, "-device", "nvme-subsys,help"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        subsys_help = result.stdout + result.stderr
        features["fdp"] = "fdp=" in subsys_help

    except (subprocess.TimeoutExpired, FileNotFoundError, OSError) as e:
        log.warning(f"Failed to detect QEMU features: {e}")

    return features


def add_args(parser: ArgumentParser):
    parser.add_argument("--nvme_img_root", type=str, default=None)
    parser.add_argument("--guest_name", type=str, default=None)


def qemu_nvme_args(nvme_img_root, features=None):
    """
    Returns list of drive-args and a string of qemu-arguments

    @param nvme_img_root: Path to store NVMe backing images
    @param features: Dict of detected QEMU features {kv, zns, fdp, pi}
    @returns drives, args
    """
    if features is None:
        features = {}

    lbads = 12

    def subsystem(id, nqn=None, aux={}):
        """
        Generate a subsystem configuration
        @param id Identifier, could be something like 'subsys0'
        @param nqn Non-qualified-name, assigned verbatim when provided
        @param aux Auxilary arguments, e.g. add {fdp: on} here, to enable fdp
        """

        args = {"id": id}
        if nqn:
            args["nqn"] = nqn
        args.update(aux)

        return [
            "-device",
            ",".join(["nvme-subsys"] + [f"{k}={v}" for k, v in args.items()]),
        ]

    def controller(id, serial, mdts, root_port, slot, subsystem=None):
        args = {
            "id": id,
            "serial": serial,
            "bus": root_port,
            "mdts": mdts,
            "ioeventfd": "on",
        }
        if subsystem:
            args["subsys"] = subsystem

        return [
            "-device",
            f"pcie-root-port,id={root_port},chassis={slot},slot={slot}",
            "-device",
            ",".join(["nvme"] + [f"{k}={v}" for k, v in args.items()]),
        ]

    def namespace(controller_id, nsid, aux={}):
        """Returns qemu-arguments for a namespace configuration"""

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
            **aux,
        }

        return drive, [
            "-drive",
            ",".join(f"{k}={v}" for k, v in drive.items()),
            "-device",
            ",".join(
                ["nvme-ns"] + [f"{k}={v}" for k, v in controller_namespace.items()]
            ),
        ]

    # =========================================================================
    # Feature flags — auto-detected from QEMU binary
    # =========================================================================
    ENABLE_ZNS = features.get("zns", False)  # since QEMU 6.0
    ENABLE_KV = features.get("kv", False)  # requires patches
    ENABLE_FDP = features.get("fdp", False)  # since QEMU 8.0
    ENABLE_PI = features.get("pi", False)  # standard feature

    # =========================================================================
    # NVMe Configuration - comment out lines to disable controllers/namespaces
    # =========================================================================
    # Format: (ctrl_id, serial, mdts, slot, subsys, [(nsid, ns_attrs), ...])
    #   - subsys: None, or (subsys_id, subsys_attrs) for FDP etc.
    # =========================================================================

    zns_attrs = {
        "zoned": "on",
        "zoned.zone_size": "32M",
        "zoned.zone_capacity": "28M",
        "zoned.max_active": 256,
        "zoned.max_open": 256,
        "zoned.zrwas": 32 << lbads,
        "zoned.zrwafg": 16 << lbads,
        "zoned.numzrwa": 256,
    }
    fdp_subsys = (
        "beef0005",
        {"fdp": "on", "fdp.nruh": "8", "fdp.nrg": "32", "fdp.runs": "40960"},
    )

    # =========================================================================
    # NVMe Configuration - one namespace per controller to avoid Linux kernel
    # namespace ordering issues. Each controller gets a single namespace.
    # =========================================================================

    nvme_config = [
        # nvme0: NVM
        ("nvme0", "beef0000", 7, 1, None, [(1, {})]),
    ]

    slot = 2  # Track slot numbers for additional controllers

    # nvme1: ZNS
    if ENABLE_ZNS:
        nvme_config.append(("nvme1", "beef0001", 7, slot, None, [(1, zns_attrs)]))
        slot += 1

    # nvme2: KV
    if ENABLE_KV:
        nvme_config.append(("nvme2", "beef0002", 7, slot, None, [(1, {"kv": "on"})]))
        slot += 1

    # nvme3: NVM (fabrics testing)
    nvme_config.append(("nvme3", "beef0003", 7, slot, None, [(1, {})]))
    slot += 1

    # nvme4: NVM (large MDTS)
    nvme_config.append(("nvme4", "beef0004", 0, slot, None, [(1, {})]))
    slot += 1

    # nvme5: FDP
    if ENABLE_FDP:
        nvme_config.append(
            ("nvme5", "beef0005", 7, slot, fdp_subsys, [(1, {"fdp.ruhs": "'0;5;6;7'"})])
        )
        slot += 1

    # nvme6-10: PI namespaces (one per controller)
    if ENABLE_PI:
        nvme_config.append(
            ("nvme6", "beef0006", 7, slot, None, [(1, {"ms": 8, "pi": 1})])
        )
        slot += 1
        nvme_config.append(
            ("nvme7", "beef0007", 7, slot, None, [(1, {"ms": 8, "pi": 2})])
        )
        slot += 1
        nvme_config.append(
            ("nvme8", "beef0008", 7, slot, None, [(1, {"ms": 8, "pi": 3})])
        )
        slot += 1
        nvme_config.append(
            ("nvme9", "beef0009", 7, slot, None, [(1, {"ms": 8, "mset": 1, "pi": 1})])
        )
        slot += 1
        nvme_config.append(
            ("nvme10", "beef0010", 7, slot, None, [(1, {"ms": 16, "pi": 1, "pif": 2})])
        )
        slot += 1

    # =========================================================================
    # Build QEMU arguments from configuration
    # =========================================================================

    drives = []
    nvme = []

    created_subsys = set()
    for ctrl_id, serial, mdts, slot, subsys, namespaces in nvme_config:
        # Create subsystem if needed
        subsys_id = None
        if subsys:
            subsys_id, subsys_attrs = subsys
            if subsys_id not in created_subsys:
                nvme += subsystem(subsys_id, aux=subsys_attrs)
                created_subsys.add(subsys_id)

        # Create controller on its own root port
        root_port = f"pcie_root_port{slot}"
        nvme += controller(ctrl_id, serial, mdts, root_port, slot, subsys_id)

        # Create namespaces
        for nsid, ns_attrs in namespaces:
            drv, qemu_dev = namespace(ctrl_id, nsid, ns_attrs)
            nvme += qemu_dev
            drives.append(drv)

    return drives, nvme


def main(args, cijoe):
    """Start a qemu guest"""

    drive_size = "8G"
    guest_name = args.guest_name or cijoe.getconf("qemu.default_guest")
    if not guest_name:
        log.error("missing config value(qemu.guest_name)")
        return 1

    guest = Guest(cijoe, cijoe.config, guest_name)

    nvme_img_root = Path(args.nvme_img_root or guest.guest_path)

    # Detect QEMU NVMe feature support
    system_label = guest.guest_config.get("system_label", "x86_64")
    qemu_bin = cijoe.getconf(
        f"qemu.systems.{system_label}.bin", f"qemu-system-{system_label}"
    )
    features = detect_qemu_nvme_features(qemu_bin)
    log.info(f"Detected QEMU NVMe features ({qemu_bin}): {features}")

    drives, nvme_args = qemu_nvme_args(nvme_img_root, features)

    # Check that the backing-storage exists, create them if they do not
    for drive in drives:
        err, _ = cijoe.run_local(f"[ -f {drive['file']} ]")
        if err:
            guest.image_create(drive["file"], drive["format"], drive_size)
        err, _ = cijoe.run_local(f"[ -f {drive['file']} ]")

    err = guest.start(extra_args=nvme_args)
    if err:
        log.error(f"guest.start() : err({err})")
        return err

    started = guest.is_up()
    if not started:
        log.error("guest.is_up() : False")
        return errno.EAGAIN

    return 0
