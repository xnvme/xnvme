#!/usr/bin/env python3
"""
Bring up an NVMe TCP transport target
=====================================

Configure an NVMe target listening on a TCP transport. Two providers are
supported via ``--provider``:

* ``spdk`` (default): use the SPDK ``nvmf_tgt`` application driven through
  ``rpc.py`` to export a locally attached PCIe NVMe device.
* ``linux``: use the Linux kernel ``nvmet`` driver through configfs to
  export an existing ``/dev/nvmeXn1``.

In both cases the device to export is read from the cijoe config entry
labelled ``fabrics`` (legacy label name).

Retargetable: True
"""
import errno
import logging as log
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--provider",
        choices=["spdk", "linux"],
        default="spdk",
        help="Target provider: SPDK nvmf_tgt or Linux kernel nvmet",
    )
    parser.add_argument(
        "--traddr",
        type=str,
        default="127.0.0.1",
        help="Transport address (IP) for the listener",
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
    parser.add_argument(
        "--adrfam",
        type=str,
        default="ipv4",
        help="Address family",
    )


def _get_transport_device(cijoe):
    """Return the first device labelled for NVMe transport export."""

    for device in cijoe.getconf("devices", []):
        if "fabrics" in device.get("labels", []):
            return device
    return None


def _run_all(cijoe, commands):
    """Run commands in order; return (err, failing_cmd) or (0, None)."""

    for cmd in commands:
        err, _ = cijoe.run(cmd)
        if err:
            return err, cmd
    return 0, None


def _start_spdk(args, cijoe):
    """Configure the SPDK NVMe TCP target."""

    xnvme_repos = cijoe.getconf("xnvme.repository.sync.remote_path", None)
    if not xnvme_repos:
        log.error("FAILED: 'xnvme.repository.sync.remote_path' not set")
        return errno.EINVAL

    device = _get_transport_device(cijoe)
    if not device:
        log.error("FAILED: no device labelled 'fabrics' in CIJOE config")
        return errno.ENOENT

    pcie_id = device["pcie_id"]
    subnqn = device["subnqn"]

    spdk_path = Path(xnvme_repos) / "subprojects" / "spdk"
    rpc = spdk_path / "scripts" / "rpc.py"
    nvmf_tgt = spdk_path / "build" / "bin" / "nvmf_tgt"
    nvmf_tgt_src = spdk_path / "app" / "nvmf_tgt"

    drivers = [
        "modprobe nvme",
        "modprobe nvmet",
        "modprobe nvmet_tcp",
        "xnvme-driver",
    ]
    err, cmd = _run_all(cijoe, drivers)
    if err:
        log.error("FAILED: driver issue: %s (errno=%d)", cmd, err)
        return err

    cijoe.run("pkill -f nvmf_tgt || true")

    subsystem = [
        f"test -x {nvmf_tgt} || make -C {nvmf_tgt_src}",
        f"(nohup {nvmf_tgt} -m [1] > nvmf_tgt.out 2> nvmf_tgt.err < /dev/null &)",
        "sleep 3",
        f"{rpc} nvmf_create_transport -t {args.trtype} -u 16384 -m 8 -c 8192",
        f"{rpc} bdev_nvme_attach_controller -b Nvme0 -t PCIe -a {pcie_id} -U",
        f"{rpc} nvmf_create_subsystem {subnqn} "
        f"-a -s SPDK00000000000001 -d Controller1 -p",
        f"{rpc} nvmf_subsystem_add_ns {subnqn} Nvme0n1",
        f"{rpc} nvmf_subsystem_add_listener {subnqn} "
        f"-t {args.trtype} -a {args.traddr} -s {args.trsvcid} -f {args.adrfam}",
    ]
    err, cmd = _run_all(cijoe, subsystem)
    if err:
        log.error("FAILED: subsystem creation: %s (errno=%d)", cmd, err)
        return err

    log.info("spdk target up: %s @ %s:%s", subnqn, args.traddr, args.trsvcid)
    return 0


def _start_linux(args, cijoe):
    """Configure the Linux kernel NVMe TCP target through nvmet/configfs."""

    device = _get_transport_device(cijoe)
    if not device:
        log.error("FAILED: no device labelled 'fabrics' in CIJOE config")
        return errno.ENOENT

    subnqn = device["subnqn"]
    dev_path = device.get("device_path")
    if not dev_path:
        log.error("FAILED: device missing 'device_path' (local /dev/nvmeXn1)")
        return errno.EINVAL
    nvmet = "/sys/kernel/config/nvmet"

    drivers = [
        "xnvme-driver reset",
        "modprobe nvme",
        "modprobe nvmet",
        "modprobe nvmet_tcp",
    ]
    err, cmd = _run_all(cijoe, drivers)
    if err:
        log.error("FAILED: driver issue: %s (errno=%d)", cmd, err)
        return err

    subsystem = [
        "mountpoint -q /sys/kernel/config "
        "|| mount -t configfs none /sys/kernel/config",
        f"mkdir -p {nvmet}/subsystems/{subnqn}",
        f"echo 1 > {nvmet}/subsystems/{subnqn}/attr_allow_any_host",
        f"mkdir -p {nvmet}/subsystems/{subnqn}/namespaces/1",
        f"echo -n {dev_path} > "
        f"{nvmet}/subsystems/{subnqn}/namespaces/1/device_path",
        f"echo 1 > {nvmet}/subsystems/{subnqn}/namespaces/1/enable",
        f"mkdir -p {nvmet}/ports/1",
        f"echo {args.traddr} > {nvmet}/ports/1/addr_traddr",
        f"echo {args.trtype} > {nvmet}/ports/1/addr_trtype",
        f"echo {args.trsvcid} > {nvmet}/ports/1/addr_trsvcid",
        f"echo {args.adrfam} > {nvmet}/ports/1/addr_adrfam",
        f"ln -s {nvmet}/subsystems/{subnqn} " f"{nvmet}/ports/1/subsystems/{subnqn}",
    ]
    err, cmd = _run_all(cijoe, subsystem)
    if err:
        log.error("FAILED: subsystem creation: %s (errno=%d)", cmd, err)
        return err

    log.info("linux target up: %s @ %s:%s", subnqn, args.traddr, args.trsvcid)
    return 0


def main(args, cijoe):
    """Bring up an NVMe TCP target using the selected provider."""

    if args.provider == "spdk":
        return _start_spdk(args, cijoe)
    if args.provider == "linux":
        return _start_linux(args, cijoe)
    log.error("unknown provider: %s", args.provider)
    return errno.EINVAL
