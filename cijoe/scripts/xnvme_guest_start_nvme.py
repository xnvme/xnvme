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

When the config has a '[rocm_ernic]' section, a rocm-ernic emulated RDMA NIC is
attached as well, over vfio-user, with its server built and started on the
initiator. Its backend is taken from 'rocm_ernic.backend'.

Retargetable: false
-------------------
"""
import errno
import logging as log
import subprocess
from argparse import ArgumentParser
from pathlib import Path

import rocm_ernic_build

from cijoe.qemu.wrapper import Guest


def detect_qemu_nvme_features(qemu_bin):
    """
    Detect which NVMe features the QEMU binary supports by parsing device help.

    Returns dict with feature flags: {kv, zns, fdp, pi}, or None when the probe
    could not run or did not find both zoned and pi, which every supported QEMU
    has. The caller must treat None as fatal: a guest built from a guessed
    feature set has its controllers on other BDFs than the config describes,
    and the test suite then runs against the wrong devices.
    """
    features = {"kv": False, "zns": False, "fdp": False, "pi": False}

    try:
        # Check nvme-ns device properties
        result = subprocess.run(
            [qemu_bin, "-device", "nvme-ns,help"],
            capture_output=True,
            text=True,
            timeout=60,
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
            timeout=60,
        )
        subsys_help = result.stdout + result.stderr
        features["fdp"] = "fdp=" in subsys_help

    except (subprocess.TimeoutExpired, FileNotFoundError, OSError) as e:
        log.error(f"Failed to detect QEMU features ({qemu_bin}): {e}")
        return None

    # zoned and pi are in every QEMU since 6.0, the fork included; their
    # absence means the probe read something other than the device help
    if not (features["zns"] and features["pi"]):
        log.error(f"QEMU feature probe ({qemu_bin}) is not credible: {features}")
        return None

    return features


def add_args(parser: ArgumentParser):
    parser.add_argument("--nvme_img_root", type=str, default=None)
    parser.add_argument("--guest_name", type=str, default=None)
    parser.add_argument(
        "--timeout",
        type=int,
        default=180,
        help="Seconds to wait for the guest to reach its login prompt.",
    )


def qemu_nvme_args(nvme_img_root, features=None, vfu_socket=None):
    """
    Returns list of drive-args and a string of qemu-arguments

    @param nvme_img_root: Path to store NVMe backing images
    @param features: Dict of detected QEMU features {kv, zns, fdp, pi}
    @param vfu_socket: When set (and native KV is absent), emit an external
        vfu_kvssd KV device attached over vfio-user, listening on this socket.
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
        # Bypass the host page cache. With the default writeback, a FLUSH has
        # to sync every write the guest made since the last one, and on a slow
        # host disk that outlasts the guest's 30 s I/O timeout.
        drive = {
            "id": drive_id,
            "file": str(nvme_img_root / f"{drive_id}.img"),
            "format": "raw",
            "if": "none",
            "discard": "on",
            "detect-zeroes": "unmap",
            "cache": "none",
            "aio": "io_uring",
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

    # nvme2: KV. Native nvme-ns kv=on if QEMU supports it, else external
    # vfu_kvssd over vfio-user. Either keeps the slot so downstream BDFs stay
    # stable.
    if ENABLE_KV:
        nvme_config.append(("nvme2", "beef0002", 7, slot, None, [(1, {"kv": "on"})]))
        slot += 1
    elif vfu_socket:
        nvme_config.append(("vfu_kvssd", vfu_socket, None, slot, None, None))
        slot += 1

    # nvme3: NVM (fabrics testing)
    nvme_config.append(("nvme3", "beef0003", 7, slot, None, [(1, {})]))
    slot += 1

    # nvme4: NVM, large MDTS. Upstream QEMU rejects mdts=0, so cap to a finite
    # value there; ENABLE_KV distinguishes upstream from the fork.
    large_mdts = 0 if ENABLE_KV else 9
    nvme_config.append(("nvme4", "beef0004", large_mdts, slot, None, [(1, {})]))
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
        # vfu_kvssd over vfio-user; JSON spec is single-quoted because cijoe
        # runs qemu via a shell.
        if ctrl_id == "vfu_kvssd":
            root_port = f"pcie_root_port{slot}"
            nvme += [
                "-device",
                f"pcie-root-port,id={root_port},chassis={slot},slot={slot}",
            ]
            vfio_dev = (
                '{"driver":"vfio-user-pci","socket":{"type":"unix","path":"'
                + serial
                + '"},"bus":"'
                + root_port
                + '"}'
            )
            nvme += ["-device", f"'{vfio_dev}'"]
            continue

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


def setup_vfu_kvssd(cijoe):
    """Ensure the vfu_kvssd server is running; return its socket path, or None
    when '[kvssd]' is absent. Fetches the binary from 'url' on first use and
    launches the server detached."""
    binary = cijoe.getconf("kvssd.binary", None)
    url = cijoe.getconf("kvssd.url", None)
    if not (binary or url):
        return None

    socket = cijoe.getconf("kvssd.socket", "/tmp/vfu_kvssd.sock")
    log_path = Path(socket).with_suffix(".log")

    need_fetch = True
    if binary:
        err, _ = cijoe.run_local(f"[ -x {binary} ]")
        need_fetch = bool(err)
    if need_fetch:
        if not binary:
            binary = Path(socket).with_name("vfu_kvssd")
        if not url:
            log.error("vfu_kvssd binary missing and no 'kvssd.url' to fetch it")
            return None
        err, _ = cijoe.run_local(
            f"curl -fL --retry 3 -o {binary} {url} && chmod +x {binary}"
        )
        if err:
            log.error(f"failed to fetch vfu_kvssd from {url}")
            return None

    # Replace any stale server bound to this socket; VFU_KVSSD_TRACE bundles a
    # control/admin/IO log so probes can be inspected post-mortem.
    cijoe.run_local(f"pkill -f '{binary} -s {socket}' || true")
    cijoe.run_local(f"rm -f {socket}")
    err, _ = cijoe.run_local(
        f"setsid env VFU_KVSSD_TRACE=1 {binary} -s {socket} > {log_path} 2>&1 "
        "< /dev/null & echo started"
    )
    if err:
        log.error(f"failed to start vfu_kvssd: err({err})")
        return None

    err, _ = cijoe.run_local(
        f"for _ in $(seq 1 100); do [ -S {socket} ] && break; sleep 0.1; done; "
        f"[ -S {socket} ]"
    )
    if err:
        log.error(f"vfu_kvssd did not create socket {socket}; see {log_path}")
        return None

    log.info(f"vfu_kvssd listening on {socket}")
    return socket


def setup_rocm_ernic(cijoe):
    """Ensure the rocm-ernic server is running; return its socket path, or None
    when '[rocm_ernic]' is absent or the server could not be built or started.
    Builds the server on first use and launches it detached, replacing any stale
    server on the socket, since QEMU's vfio-user session does not survive one."""

    if not cijoe.getconf("rocm_ernic", None):
        return None

    binary = rocm_ernic_build.build(cijoe)
    if not binary:
        return None

    socket = cijoe.getconf("rocm_ernic.socket", "/tmp/vfio-user-rocm-ernic.sock")
    backend = cijoe.getconf("rocm_ernic.backend", "loopback")
    log_level = cijoe.getconf("rocm_ernic.log_level", "warn")
    log_path = Path(socket).with_suffix(".log")

    cijoe.run_local(f"pkill -f '{binary} --socket {socket}' || true")
    cijoe.run_local(f"rm -f {socket}")
    err, _ = cijoe.run_local(
        f"setsid {binary} --socket {socket} --backend '{backend}' "
        f"--log-level {log_level} > {log_path} 2>&1 < /dev/null & echo started"
    )
    if err:
        log.error(f"failed to start rocm-ernic: err({err})")
        return None

    err, _ = cijoe.run_local(
        f"for _ in $(seq 1 300); do [ -S {socket} ] && break; sleep 0.1; done; "
        f"[ -S {socket} ]"
    )
    if err:
        log.error(f"rocm-ernic did not create socket {socket}; see {log_path}")
        return None

    log.info(f"rocm-ernic ({backend}) listening on {socket}")
    return socket


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
    if features is None:
        return errno.EINVAL
    log.info(f"Detected QEMU NVMe features ({qemu_bin}): {features}")

    # KV source: native nvme-ns kv=on (fork) or external vfu_kvssd over vfio-user
    # (upstream QEMU >= 10.1, needs a shareable guest-RAM backend).
    extra_args = []
    vfu_socket = None
    if not features.get("kv"):
        vfu_socket = setup_vfu_kvssd(cijoe)

    ernic_socket = setup_rocm_ernic(cijoe)
    if cijoe.getconf("rocm_ernic", None) and not ernic_socket:
        return errno.EIO

    if vfu_socket or ernic_socket:
        mem = guest.guest_config.get("system_args", {}).get("kwa", {}).get("m", "6G")
        extra_args += [
            "-machine",
            "memory-backend=mem0",
            "-object",
            f"memory-backend-memfd,id=mem0,size={mem},share=on",
        ]

    drives, nvme_args = qemu_nvme_args(nvme_img_root, features, vfu_socket)
    extra_args += nvme_args

    # On the root bus, as rocm-ernic's own launchers attach it; rombar=0 drops the
    # ROM BAR vfio-user-pci would otherwise advertise. The JSON spec is
    # single-quoted because cijoe runs qemu via a shell.
    if ernic_socket:
        ernic_dev = (
            '{"driver":"vfio-user-pci","rombar":0,'
            '"socket":{"type":"unix","path":"' + ernic_socket + '"}}'
        )
        extra_args += ["-device", f"'{ernic_dev}'"]

    # Check that the backing-storage exists, create them if they do not
    for drive in drives:
        err, _ = cijoe.run_local(f"[ -f {drive['file']} ]")
        if err:
            guest.image_create(drive["file"], drive["format"], drive_size)
        err, _ = cijoe.run_local(f"[ -f {drive['file']} ]")

    err = guest.start(extra_args=extra_args)
    if err:
        log.error(f"guest.start() : err({err})")
        return err

    started = guest.is_up(timeout=args.timeout)
    if not started:
        log.error("guest.is_up() : False")
        return errno.EAGAIN

    return 0
