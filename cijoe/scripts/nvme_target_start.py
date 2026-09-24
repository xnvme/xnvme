#!/usr/bin/env python3
"""
Bring up an NVMe transport target
=====================================

Configure an NVMe target listening on a transport. Two providers are
supported via ``--nvme-provider``:

* ``spdk`` (default): use the SPDK ``nvmf_tgt`` application driven through
  ``rpc.py`` to export a locally attached PCIe NVMe device.
* ``linux``: use the Linux kernel ``nvmet`` driver through configfs to
  export an existing ``/dev/nvmeXn1``, or pass through ``/dev/nvmeX``.

Each cijoe config entry labelled ``fabrics`` (legacy label name) becomes one
subsystem, listening on the ``uri`` of the entry, e.g. ``127.0.0.1:4420``.
Passthrough is off unless the entry sets ``passthrough = true``.

Retargetable: True
"""
import errno
import logging as log
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--nvme-provider",
        choices=["spdk", "linux"],
        default="spdk",
        help="NVMe target provider: SPDK nvmf_tgt or Linux kernel nvmet",
    )
    parser.add_argument(
        "--nvme-trtype",
        type=str,
        default="tcp",
        choices=["tcp", "rdma"],
        help="Transport type for the NVMe target",
    )
    parser.add_argument(
        "--nvme-adrfam",
        type=str,
        default="ipv4",
        help="Address family",
    )
    parser.add_argument(
        "--transport-name",
        type=str,
        default=None,
        help="Transport to use for cijoe.run() commands",
    )


def _get_transport_devices(cijoe):
    """Return all devices labelled for NVMe transport export."""

    return [
        device
        for device in cijoe.getconf("devices", [])
        if "fabrics" in device.get("labels", [])
    ]


def _run_all(cijoe, commands, transport_name):
    """Run commands in order; return (err, failing_cmd) or (0, None)."""

    for cmd in commands:
        err, _ = cijoe.run(cmd, transport_name=transport_name)
        if err:
            return err, cmd
    return 0, None


def _start_spdk(args, cijoe):
    """Configure the SPDK NVMe target."""

    xnvme_repos = cijoe.getconf("xnvme.repository.sync.remote_path", None)
    if not xnvme_repos:
        log.error("FAILED: 'xnvme.repository.sync.remote_path' not set")
        return errno.EINVAL

    devices = _get_transport_devices(cijoe)
    if not devices:
        log.error("FAILED: no device labelled 'fabrics' in CIJOE config")
        return errno.ENOENT

    spdk_path = Path(xnvme_repos) / "subprojects" / "spdk"
    rpc = spdk_path / "scripts" / "rpc.py"
    nvmf_tgt = spdk_path / "build" / "bin" / "nvmf_tgt"
    nvmf_tgt_src = spdk_path / "app" / "nvmf_tgt"

    # nvme_tcp/nvme_rdma is the kernel host driver loaded for the initiator-side
    # `nvme discover` in the probe step; nvmf_tgt itself runs in userspace and
    # the PCIe device is rebound to vfio-pci by xnvme-driver, so nvmet and
    # nvmet_tcp/nvmet_rdma are not needed on the SPDK target path.
    # RDMA requires kernel modules and supporting software (e.g. libibverbs) to be
    # installed and configured on both the initiator and target. An error here may
    # reflect incomplete RDMA setup rather than a fault in this script.
    drivers = [
        f"modprobe nvme_{args.nvme_trtype}",
        "xnvme-driver",
    ]
    err, cmd = _run_all(cijoe, drivers, args.transport_name)
    if err:
        log.error("FAILED: driver issue: %s (errno=%d)", cmd, err)
        return err

    cijoe.run("pkill -f nvmf_tgt || true", transport_name=args.transport_name)

    subsystem = [
        f"test -x {nvmf_tgt} || make -C {nvmf_tgt_src}",
        f"(nohup {nvmf_tgt} -m [1] > nvmf_tgt.out 2> nvmf_tgt.err < /dev/null &)",
        "sleep 3",
        f"{rpc} nvmf_create_transport -t {args.nvme_trtype} -u 16384 -m 8 -c 8192",
    ]
    for idx, device in enumerate(devices):
        traddr, trsvcid = device["uri"].rsplit(":", 1)
        subnqn = device["subnqn"]
        passthrough = " -p" if device.get("passthrough", False) else ""
        subsystem += [
            f"{rpc} bdev_nvme_attach_controller -b Nvme{idx} -t PCIe -a {device['pcie_id']} -U",
            f"{rpc} nvmf_create_subsystem {subnqn} "
            f"-a -s SPDK{idx + 1:014d} -d Controller{idx + 1}{passthrough}",
            f"{rpc} nvmf_subsystem_add_ns {subnqn} Nvme{idx}n1",
            f"{rpc} nvmf_subsystem_add_listener {subnqn} "
            f"-t {args.nvme_trtype} -a {traddr} -s {trsvcid} -f {args.nvme_adrfam}",
        ]
    err, cmd = _run_all(cijoe, subsystem, args.transport_name)
    if err:
        log.error("FAILED: subsystem creation: %s (errno=%d)", cmd, err)
        return err

    for device in devices:
        log.info("spdk target up: %s @ %s", device["subnqn"], device["uri"])
    return 0


def _start_linux(args, cijoe):
    """Configure the Linux kernel NVMe target through nvmet/configfs."""

    devices = _get_transport_devices(cijoe)
    if not devices:
        log.error("FAILED: no device labelled 'fabrics' in CIJOE config")
        return errno.ENOENT

    for device in devices:
        path = "ctrl_path" if device.get("passthrough", False) else "device_path"
        if not device.get(path):
            log.error("FAILED: device missing '%s'", path)
            return errno.EINVAL
    nvmet = "/sys/kernel/config/nvmet"

    drivers = [
        "xnvme-driver reset",
        "modprobe nvme",
        "modprobe nvmet",
        f"modprobe nvmet_{args.nvme_trtype}",
    ]
    err, cmd = _run_all(cijoe, drivers, args.transport_name)
    if err:
        log.error("FAILED: driver issue: %s (errno=%d)", cmd, err)
        return err

    subsystem = [
        "mountpoint -q /sys/kernel/config "
        "|| mount -t configfs none /sys/kernel/config",
    ]
    for portid, device in enumerate(devices, start=1):
        traddr, trsvcid = device["uri"].rsplit(":", 1)
        subnqn = device["subnqn"]
        port = f"{nvmet}/ports/{portid}"
        subsystem += [
            f"mkdir -p {nvmet}/subsystems/{subnqn}",
            f"echo 1 > {nvmet}/subsystems/{subnqn}/attr_allow_any_host",
        ]
        if device.get("passthrough", False):
            passthru = f"{nvmet}/subsystems/{subnqn}/passthru"
            subsystem += [
                f"echo -n {device['ctrl_path']} > {passthru}/device_path",
                f"echo 1 > {passthru}/enable",
            ]
        else:
            ns = f"{nvmet}/subsystems/{subnqn}/namespaces/1"
            subsystem += [
                f"mkdir -p {ns}",
                f"echo -n {device['device_path']} > {ns}/device_path",
                f"echo 1 > {ns}/enable",
            ]
        subsystem += [
            f"mkdir -p {port}",
            f"echo {traddr} > {port}/addr_traddr",
            f"echo {args.nvme_trtype} > {port}/addr_trtype",
            f"echo {trsvcid} > {port}/addr_trsvcid",
            f"echo {args.nvme_adrfam} > {port}/addr_adrfam",
            f"ln -s {nvmet}/subsystems/{subnqn} {port}/subsystems/{subnqn}",
        ]
    err, cmd = _run_all(cijoe, subsystem, args.transport_name)
    if err:
        log.error("FAILED: subsystem creation: %s (errno=%d)", cmd, err)
        return err

    for device in devices:
        log.info("linux target up: %s @ %s", device["subnqn"], device["uri"])
    return 0


def main(args, cijoe):
    """Bring up an NVMe target using the selected provider."""

    if args.nvme_provider == "spdk":
        return _start_spdk(args, cijoe)
    if args.nvme_provider == "linux":
        return _start_linux(args, cijoe)
    log.error("unknown provider: %s", args.nvme_provider)
    return errno.EINVAL
