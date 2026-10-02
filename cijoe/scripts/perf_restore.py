#!/usr/bin/env python3
"""
Put a host back after benchmarks
================================

Rebinds the devices to the drivers they had, through ``devbind``, and restores
the cpufreq governor, both read from the record perf_prepare left on the
target in /var/tmp/xnvme-perf-prepare.json, which is removed once everything
is back. Hugepages are left reserved; nothing else on the host is touched.

Retargetable: True
------------------
"""
import json
import logging as log

SYS_CPU = "/sys/devices/system/cpu"
RECORD = "/var/tmp/xnvme-perf-prepare.json"
BOOT_ID = "/proc/sys/kernel/random/boot_id"


def main(args, cijoe):
    err, state = cijoe.run(f"cat {RECORD}")
    if err:
        log.error(f"no {RECORD} on the host; nothing to restore")
        return err
    record = json.loads(state.output())
    first_err = 0

    err, state = cijoe.run(f"cat {BOOT_ID}")
    if (
        not err
        and record.get("boot_id")
        and record["boot_id"] != state.output().strip()
    ):
        log.info(
            f"{RECORD} is from an earlier boot; the reboot restored the host, removing it"
        )
        cijoe.run(f"rm -f {RECORD}")
        return 0

    for bdf, driver in record.get("drivers", {}).items():
        if driver == "-":
            err, _ = cijoe.run(f"devbind --device {bdf} --unbind")
        else:
            err, _ = cijoe.run(f"devbind --device {bdf} --bind {driver}")
        if err:
            log.error(f"{bdf}: devbind back to {driver} failed: err({err})")
            first_err = first_err or err
        else:
            log.info(f"{bdf}: back on {driver}")

    frequency = record.get("frequency")
    if frequency:
        # min first: the old min is below the pinned value, so it is accepted,
        # and the old max is then above the restored min
        for knob, value in (
            ("scaling_min_freq", frequency["min"]),
            ("scaling_max_freq", frequency["max"]),
        ):
            err, _ = cijoe.run(
                f"for f in {SYS_CPU}/cpu*/cpufreq/{knob}; do echo {value} > $f; done"
            )
            if err:
                log.error(f"restoring {knob}={value} failed: err({err})")
                first_err = first_err or err
        if not err:
            log.info(
                f"frequency: range back to {frequency['min']}-{frequency['max']} kHz"
            )

    governor = record.get("governor")
    if governor:
        err, _ = cijoe.run(
            f"for g in {SYS_CPU}/cpu*/cpufreq/scaling_governor; do echo {governor} > $g; done"
        )
        if err:
            log.error(f"restoring governor '{governor}' failed: err({err})")
            first_err = first_err or err
        else:
            log.info(f"governor: back to {governor}")

    if not first_err:
        cijoe.run(f"rm -f {RECORD}")
    return first_err
