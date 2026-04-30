#!/usr/bin/env python3
"""
Collect uPCIe IOMMU overhead benchmark inputs
=============================================

This script runs one driver in the current boot configuration. UIO and VFIO
comparisons are assembled later from two separate CIJOE outputs.
"""
import errno
import logging as log
import re
import shlex
import shutil
import time
import traceback
from argparse import ArgumentParser
from itertools import product
from pathlib import Path

from cijoe.core.resources import dict_from_yamlfile

GROUP = "upcie-iommu-overhead"
RUN_DEFAULTS = {
    "runtime": 10,
    "repeat": 3,
    "cpumask": "0x1",
    "workload_pause": 5,
}
IOMMU_ENABLED_PATTERNS = [
    r"\bDMAR:\s+IOMMU enabled\b",
    r"\bAMD-Vi:\s+IOMMU enabled\b",
    r"\bIntel-IOMMU:\s+enabled\b",
    r"\bIOMMU enabled\b",
    r"\biommu:\s+Default domain type:\b",
    r"\bAdding to iommu group\b",
    r"\bDMAR:\s+Intel\(R\) Virtualization Technology for Directed I/O\b",
    r"\bDMAR:\s+dmar\d+:\s+Using Queued invalidation\b",
]
IOMMU_DISABLED_PATTERNS = [
    r"\bintel_iommu=off\b",
    r"\bamd_iommu=off\b",
    r"\biommu=off\b",
    r"\bIOMMU disabled\b",
    r"\bIOMMU not enabled\b",
]


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--runs",
        type=Path,
        default=None,
        help="Path to a yaml file describing the upcie benchmark workload matrix",
    )


def conf(cijoe, key, default=None):
    return cijoe.getconf(f"upcie_iommu_overhead.{key}", default)


def runconf(runs, key):
    return runs.get(key, RUN_DEFAULTS[key])


def repo_path(cijoe):
    return Path(cijoe.getconf("xnvme.repository.path", str(Path.cwd().parent)))


def q(value):
    return shlex.quote(str(value))


def resolve_bin(cijoe, name, default):
    path = Path(conf(cijoe, f"bins.{name}", str(repo_path(cijoe) / default)))
    if path.is_dir():
        return path / name
    return path


def workload_cases(runs):
    for workload in runs.get("workloads", []):
        pattern = workload["pattern"]
        for iosize, iodepth in product(workload["iosizes"], workload["iodepths"]):
            yield pattern, int(iosize), int(iodepth)


def run_and_copy(cijoe, cmd, output_path):
    err, state = cijoe.run(cmd)
    shutil.copyfile(state.output_fpath, output_path)
    if err:
        log.error(f"failed: {state}")
    return err


def dmesg_indicates_iommu_enabled(text):
    for pattern in IOMMU_DISABLED_PATTERNS:
        if re.search(pattern, text, re.IGNORECASE):
            return False

    return any(
        re.search(pattern, text, re.IGNORECASE) for pattern in IOMMU_ENABLED_PATTERNS
    )


def check_iommu_state(cijoe, driver):
    cmd = "sudo dmesg"

    err, state = cijoe.run(cmd)
    if err:
        log.error(f"failed: {state}")
        return err, None

    dmesg = Path(state.output_fpath).read_text(encoding="utf-8", errors="replace")
    iommu_enabled = dmesg_indicates_iommu_enabled(dmesg)

    output_fpath = state.output_fpath

    if driver == "uio_pci_generic" and iommu_enabled:
        log.error(
            "uio_pci_generic expects an IOMMU-off boot, but dmesg "
            f"indicates that IOMMU is enabled: {output_fpath}"
        )
        return errno.EINVAL, None

    if driver == "vfio-pci" and not iommu_enabled:
        log.error(
            "vfio-pci expects an IOMMU-enabled boot, but dmesg does "
            f"not indicate that IOMMU is enabled: {output_fpath}"
        )
        return errno.EINVAL, None

    return 0, dmesg


def write_dmesg_artifact(artifacts, driver, dmesg):
    output_path = artifacts / f"dmesg-iommu_LABEL={driver}_GROUP={GROUP}.txt"
    output_path.write_text(dmesg, encoding="utf-8")


def print_progress(message):
    print(f"{message}\033[K", end="\r", flush=True)


def configure_driver(cijoe, driver_script, driver, device, huge_mem):
    env = {
        "PCI_WHITELIST": device,
        "DRIVER_OVERRIDE": driver,
    }
    if huge_mem:
        env["HUGEMEM"] = str(huge_mem)

    env_args = " ".join(f"{key}={q(value)}" for key, value in env.items())
    cmd = f"sudo env {env_args} bash {q(driver_script)}"

    err, state = cijoe.run(cmd)
    if err:
        log.error(f"failed: {state}")
    return err


def reset_driver(cijoe, driver_script, device):
    cmd = f"sudo env PCI_WHITELIST={q(device)} " f"bash {q(driver_script)} reset"
    err, state = cijoe.run(cmd)
    if err:
        log.error(f"failed: {state}")
    return err


def capture_device_info(cijoe, xnvme_bin, device, nsid, output_path):
    cmd = f"sudo {q(xnvme_bin)} info --be upcie " f"--dev-nsid {q(nsid)} {q(device)}"
    return run_and_copy(cijoe, cmd, output_path)


def xnvmeperf_cmd(cijoe, runs, xnvmeperf_bin, device, pattern, iosize, iodepth):
    runtime = int(runconf(runs, "runtime"))
    cpumask = runconf(runs, "cpumask")

    return " ".join(
        [
            f"sudo {q(xnvmeperf_bin)}",
            "run",
            "--be upcie",
            f"--iopattern {q(pattern)}",
            f"--qdepth {q(iodepth)}",
            f"--iosize {q(iosize)}",
            f"--runtime {q(runtime)}",
            f"--cpumask {q(cpumask)}",
            q(device),
        ]
    )


def write_metadata(artifacts, runs, driver, device, nsid, runs_path):
    meta_path = artifacts / "upcie-iommu-overhead-meta.txt"
    values = {
        "driver": driver,
        "device": device,
        "nsid": nsid,
        "runner": "xnvmeperf",
        "runtime": runconf(runs, "runtime"),
        "repeat": runconf(runs, "repeat"),
        "cpumask": runconf(runs, "cpumask"),
        "workload_pause": runconf(runs, "workload_pause"),
        "runs": runs_path,
    }

    with meta_path.open("w") as meta:
        for key, value in values.items():
            meta.write(f"{key}={value}\n")


def prune_artifacts(artifacts):
    # CIJOE usually creates a fresh output directory, but local iteration often
    # reuses one. Keep reruns from mixing old raw inputs with new normalized data.
    for pattern in [
        "xnvmeperf-output_*.txt",
        "dmesg-iommu_*.txt",
        "xnvme-info_*.txt",
    ]:
        for path in artifacts.glob(pattern):
            path.unlink()
    normalized = artifacts / "benchmark-output-normalized.json"
    if normalized.exists():
        normalized.unlink()


def main(args, cijoe):
    if not args.runs:
        log.error("Missing path to auxiliary file describing the runs")
        return errno.EINVAL

    driver = conf(cijoe, "driver")
    device = conf(cijoe, "device")
    nsid = int(conf(cijoe, "nsid", 1))
    huge_mem = conf(cijoe, "huge_mem", None)
    runs = dict_from_yamlfile(args.runs)
    repeat = int(runconf(runs, "repeat"))
    workload_pause = int(runconf(runs, "workload_pause"))

    if not driver or not device:
        log.error("Missing upcie_iommu_overhead.driver or .device")
        return errno.EINVAL

    xnvme_bin = resolve_bin(cijoe, "xnvme", "builddir/tools/xnvme")
    xnvmeperf_bin = resolve_bin(
        cijoe, "xnvmeperf", "builddir/tools/xnvmeperf/xnvmeperf"
    )
    driver_script = resolve_bin(cijoe, "driver_script", "toolbox/xnvme-driver.sh")

    err = 0
    driver_configured = False
    cases = list(workload_cases(runs))

    try:
        err, dmesg = check_iommu_state(cijoe, driver)
        if err:
            return err

        artifacts = Path(args.output) / "artifacts"
        artifacts.mkdir(parents=True, exist_ok=True)
        prune_artifacts(artifacts)
        write_metadata(artifacts, runs, driver, device, nsid, args.runs)
        write_dmesg_artifact(artifacts, driver, dmesg)

        err = configure_driver(cijoe, driver_script, driver, device, huge_mem)
        if err:
            return err
        driver_configured = True

        info_path = artifacts / f"xnvme-info_LABEL={driver}_GROUP={GROUP}.txt"
        err = capture_device_info(cijoe, xnvme_bin, device, nsid, info_path)
        if err:
            return err

        combinations = len(cases) * repeat
        completed = 0
        print(
            "run:",
            "{",
            f"'driver': '{driver}'",
            f"'workloads': {len(cases)}",
            f"'repeat': {repeat}",
            "}",
        )
        print_progress(f"0% completed (0/{combinations})")

        for case_idx, (pattern, iosize, iodepth) in enumerate(cases, start=1):
            for rep in range(1, repeat + 1):
                workload = (
                    f"RW={pattern} IOSIZE={iosize} "
                    f"IODEPTH={iodepth} REP={rep}/{repeat}"
                )
                print_progress(
                    f"{completed / combinations * 100:.0f}% completed "
                    f"({completed}/{combinations}); running {workload}"
                )

                output_path = artifacts / (
                    f"xnvmeperf-output_IOSIZE={iosize}_IODEPTH={iodepth}_"
                    f"LABEL={driver}_GROUP={GROUP}_RW={pattern}_REP={rep}.txt"
                )
                cmd = xnvmeperf_cmd(
                    cijoe, runs, xnvmeperf_bin, device, pattern, iosize, iodepth
                )
                err = run_and_copy(cijoe, cmd, output_path)
                if err:
                    return err

                completed += 1
                print_progress(
                    f"{completed / combinations * 100:.0f}% completed "
                    f"({completed}/{combinations}); finished {workload}"
                )

            if workload_pause > 0 and case_idx < len(cases):
                time.sleep(workload_pause)

        print(f"100% completed ({completed}/{combinations})", flush=True)

    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        return 1
    finally:
        if driver_configured:
            reset_driver(cijoe, driver_script, device)

    return err
