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

* /dev/ng2n1 -- nvm (large mdts)
* /dev/ng3n1 -- fdp-enabled subsystem and nvm namespace

Using the the 'ng' device-handle, as it is always available, whereas 'nvme' are
only for block-devices. Regardless, the above is just to illustrate one
possible "appearance" of the devices in Linux.

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

    def controller(
        id, serial, mdts, downstream_bus, upstream_bus, controller_slot, subsystem=None
    ):
        args = {
            "id": id,
            "serial": serial,
            "bus": downstream_bus,
            "mdts": mdts,
            "ioeventfd": "on",
        }
        if subsystem:
            args["subsys"] = subsystem

        return [
            "-device",
            f"xio3130-downstream,id={downstream_bus},bus={upstream_bus},chassis=2,slot={controller_slot}",
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
    # Feature flags - ZNS/FDP/PI are standard QEMU features (always enabled),
    # KV requires QEMU patches (auto-detected)
    # =========================================================================
    ENABLE_ZNS = True  # Zoned Namespaces - standard since QEMU 6.0
    ENABLE_KV = features.get("kv", False)  # Key-Value - requires QEMU patches
    ENABLE_FDP = True  # Flexible Data Placement - standard since QEMU 8.0
    ENABLE_PI = True  # Protection Information - standard QEMU feature

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
        "subsys0",
        {"fdp": "on", "fdp.nruh": "8", "fdp.nrg": "32", "fdp.runs": "40960"},
    )

    # Build nvme0 namespaces based on feature flags
    nvme0_namespaces = [(1, {})]  # NVM always included
    if ENABLE_ZNS:
        nvme0_namespaces.append((2, zns_attrs))
    if ENABLE_KV:
        nvme0_namespaces.append((3, {"kv": "on"}))

    # Build nvme4 (PI) namespaces
    nvme4_namespaces = []
    if ENABLE_PI:
        nvme4_namespaces = [
            (1, {"ms": 8, "pi": 1}),  # PI type 1
            (2, {"ms": 8, "pi": 2}),  # PI type 2
            (3, {"ms": 8, "pi": 3}),  # PI type 3
            (4, {"ms": 8, "mset": 1, "pi": 1}),  # PI type 1, extended LBA
            (5, {"ms": 16, "pi": 1, "pif": 2}),  # PI type 1, PIF 2
        ]

    nvme_config = [
        # nvme0: Functional verification (NVM, ZNS, KV)
        ("nvme0", "deadbeef", 7, 1, None, nvme0_namespaces),
        # nvme1: Fabrics testing
        ("nvme1", "adcdbeef", 7, 2, None, [(1, {})]),
        # nvme2: Large MDTS
        ("nvme2", "beefcace", 7, 3, None, [(1, {})]),
    ]

    # Add FDP controller if enabled
    if ENABLE_FDP:
        nvme_config.append(
            ("nvme3", "beefcace", 7, 4, fdp_subsys, [(1, {"fdp.ruhs": "'0;5;6;7'"})])
        )

    # Add PI controller if enabled
    if ENABLE_PI:
        nvme_config.append(("nvme4", "feebdaed", 7, 5, None, nvme4_namespaces))

    # =========================================================================
    # Build QEMU arguments from configuration
    # =========================================================================

    drives = []
    nvme = []
    nvme += ["-device", "pcie-root-port,id=pcie_root_port1,chassis=1,slot=1"]

    upstream_bus = "pcie_upstream_port1"
    nvme += ["-device", f"x3130-upstream,id={upstream_bus},bus=pcie_root_port1"]

    created_subsys = set()
    for ctrl_id, serial, mdts, slot, subsys, namespaces in nvme_config:
        # Create subsystem if needed
        subsys_id = None
        if subsys:
            subsys_id, subsys_attrs = subsys
            if subsys_id not in created_subsys:
                nvme += subsystem(subsys_id, aux=subsys_attrs)
                created_subsys.add(subsys_id)

        # Create controller
        ctrl_bus = f"pcie_downstream_port{slot}"
        nvme += controller(
            ctrl_id, serial, mdts, ctrl_bus, upstream_bus, slot, subsys_id
        )

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
