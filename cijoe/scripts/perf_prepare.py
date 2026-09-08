#!/usr/bin/env python3
"""
Prepare a host for benchmarks
=============================

Checks the IOMMU mode, pins the CPU frequency governor, reserves hugepages,
and binds the configured devices to the user-space driver, using uPCIe's
``iommu``, ``hugepages`` and ``devbind`` tools on the target. What it changes
is recorded on the target, in /var/tmp/xnvme-perf-prepare.json, so that
perf_restore can put it back from a later invocation. A host that carries
that record is already prepared; the task stops rather than overwrite it. A
record from an earlier boot describes a state a reboot already undid, so it
is discarded rather than honoured.

The IOMMU mode is verified, not set: changing it rewrites the bootloader
configuration and takes a reboot, which is not this task's to do.

Config Arguments
----------------

perf.iommu: mode 'iommu show' must report, e.g. off-for-uio or pt; unset to
            accept whatever the host is in
perf.governor: cpufreq governor to set on every CPU (default: performance)
perf.frequency_khz: pin every CPU to this frequency by setting the cpufreq
                    minimum and maximum to it; unset leaves the range alone.
                    A fixed clock below the turbo ceiling makes runs
                    comparable across boots, whose ceilings differ
perf.hugepages: 2 MiB hugepages to reserve in total; the kernel spreads them
                over the NUMA nodes
perf.driver: driver to bind the devices to (default: uio_pci_generic)
devices: list of tables with 'pci_addr' (the BDF), from the devices config

Retargetable: True
------------------
"""
import json
import logging as log

SYS_CPU = "/sys/devices/system/cpu"
SYS_PCI = "/sys/bus/pci"
RECORD = "/var/tmp/xnvme-perf-prepare.json"
BOOT_ID = "/proc/sys/kernel/random/boot_id"


def check_iommu(cijoe, wanted):
    err, state = cijoe.run("iommu show")
    if err:
        log.error(f"'iommu show' failed: err({err})")
        return err
    mode = None
    for line in state.output().splitlines():
        if line.startswith("mode:"):
            mode = line.split(":", 1)[1].strip()
    log.info(f"iommu mode: {mode}")
    if wanted and mode != wanted:
        log.error(
            f"host is in iommu mode '{mode}', the config wants '{wanted}'; reboot into it first"
        )
        return 1
    return 0


def set_governor(cijoe, governor, record):
    err, state = cijoe.run(f"cat {SYS_CPU}/cpu0/cpufreq/scaling_governor")
    if err:
        log.warning("no cpufreq governor on this host; leaving frequency alone")
        return 0
    prev = state.output().strip()
    record["governor"] = prev

    err, _ = cijoe.run(
        f"for g in {SYS_CPU}/cpu*/cpufreq/scaling_governor; do echo {governor} > $g; done"
    )
    if err:
        log.error(f"setting governor '{governor}' failed: err({err})")
        return err
    log.info(f"governor: {prev} -> {governor}")
    return 0


def pin_frequency(cijoe, khz, record):
    """Pin every CPU to 'khz' by closing the cpufreq range on it."""

    err, state = cijoe.run(
        f"cat {SYS_CPU}/cpu0/cpufreq/scaling_min_freq {SYS_CPU}/cpu0/cpufreq/scaling_max_freq"
    )
    if err:
        log.error("no cpufreq range on this host; cannot pin the frequency")
        return err
    prev_min, prev_max = state.output().split()
    record["frequency"] = {"min": prev_min, "max": prev_max}

    # max first: the new max is above the old min, so it is accepted, and the
    # new min is then below the new max
    for knob in ("scaling_max_freq", "scaling_min_freq"):
        err, _ = cijoe.run(
            f"for f in {SYS_CPU}/cpu*/cpufreq/{knob}; do echo {khz} > $f; done"
        )
        if err:
            log.error(f"writing {knob}={khz} failed: err({err})")
            return err
    err, state = cijoe.run(f"cat {SYS_CPU}/cpu0/cpufreq/scaling_cur_freq")
    log.info(
        f"frequency: pinned to {khz} kHz (cpu0 now at {state.output().strip()} kHz)"
    )
    return 0


def set_hugepages(cijoe, count):
    err, _ = cijoe.run(f"hugepages setup --size 2048 --count {count}")
    if err:
        log.error(f"'hugepages setup --count {count}' failed: err({err})")
        return err
    cijoe.run("hugepages info")
    return 0


def current_driver(cijoe, bdf):
    """The driver 'bdf' is bound to, or '-' when it has none."""

    err, state = cijoe.run(f"basename $(readlink {SYS_PCI}/devices/{bdf}/driver)")
    return state.output().strip() if not err else "-"


def bind_devices(cijoe, bdfs, driver, record):
    """Record the drivers the devices have, then bind every one to 'driver'."""

    prev = {bdf: current_driver(cijoe, bdf) for bdf in bdfs}
    record["drivers"] = prev

    for bdf in bdfs:
        err, _ = cijoe.run(f"devbind --device {bdf} --bind {driver}")
        if err:
            log.error(f"{bdf}: devbind to {driver} failed: err({err})")
            return err
        now = current_driver(cijoe, bdf)
        if now != driver:
            log.error(f"{bdf}: bound to '{now}' after devbind, wanted {driver}")
            return 1
        log.info(f"{bdf}: {prev[bdf]} -> {now}")
    return 0


def main(args, cijoe):
    devices = cijoe.getconf("devices", [])
    bdfs = [d["pci_addr"] for d in devices if "pci_addr" in d]
    if not bdfs:
        log.error("no devices with 'pci_addr' in config")
        return 1

    err, state = cijoe.run(f"cat {BOOT_ID}")
    if err:
        log.error(f"reading {BOOT_ID} failed: err({err})")
        return err
    boot_id = state.output().strip()

    err, state = cijoe.run(f"cat {RECORD}")
    if not err:
        if json.loads(state.output()).get("boot_id") == boot_id:
            log.error(
                f"{RECORD} exists: the host is already prepared; run perf_restore first"
            )
            return 1
        log.info(
            f"{RECORD} is from an earlier boot; the reboot undid it, discarding it"
        )
        cijoe.run(f"rm -f {RECORD}")

    err = check_iommu(cijoe, cijoe.getconf("perf.iommu", None))
    if err:
        return err

    record = {"boot_id": boot_id}
    err = set_governor(cijoe, cijoe.getconf("perf.governor", "performance"), record)
    if err:
        return err

    khz = cijoe.getconf("perf.frequency_khz", None)
    if khz is not None:
        err = pin_frequency(cijoe, int(khz), record)
        if err:
            return err

    count = cijoe.getconf("perf.hugepages", None)
    if count is not None:
        err = set_hugepages(cijoe, int(count))
        if err:
            return err

    err = bind_devices(
        cijoe, bdfs, cijoe.getconf("perf.driver", "uio_pci_generic"), record
    )
    if err:
        return err

    err, _ = cijoe.run(f"cat > {RECORD} << 'EOF'\n{json.dumps(record, indent=2)}\nEOF")
    if err:
        log.error(f"writing {RECORD} failed: err({err})")
    return err
