#!/usr/bin/env python3
"""
Fail a benchmark run whose xNVMe engines fall behind their native peers
=======================================================================

Reads the results benchmark_normalize wrote, and for each xNVMe engine and
native engine in one group, 'xnvme_io_uring' and 'io_uring' say, takes the
geometric mean, over the iosizes and iodepths both ran, of the xNVMe engine's
IOPS over the native one's, its samples at a point averaged first. The run
fails when a mean is below its floor, or the pair has none: 'thresholds' gives
the floors by the target's 'os.name', then by '<xnvme label>/<native label>',
as an engine's standing against its native peer differs from one OS to the
next. A floor may name a native engine of another group, for an xNVMe engine
whose group has none; an xNVMe engine with no peer at all fails the run, as
do a pair with no point both engines ran, and any engine reporting no IOPS at
a point, as fio does when its job fails.

The mean is over a run's points because a single point's ratio varies by tens
of percent between runs, so a regression confined to a few points fails only
once it is large.

Comparing engines measured in the same run, on the same machine, leaves out
how fast that machine was that day, which a floor on IOPS alone cannot. An
engine is xNVMe's when its label starts with 'xnvme' or names 'bdev_xnvme'.

Retargetable: False
-------------------
"""
import errno
import json
import logging as log
import math
from argparse import ArgumentParser
from collections import defaultdict
from pathlib import Path

import yaml

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"


def add_args(parser: ArgumentParser):
    parser.add_argument("--path", type=Path, default=None)
    parser.add_argument("--thresholds", type=Path, required=True)


def is_xnvme(label):
    return label.startswith("xnvme") or "bdev_xnvme" in label


def pairs(groups, floors):
    """The xNVMe/native pairs: those sharing a group, and those the floors name"""

    natives = defaultdict(list)
    for label, group in groups.items():
        if not is_xnvme(label):
            natives[group].append(label)

    found = {
        (label, native)
        for label, group in groups.items()
        if is_xnvme(label)
        for native in natives[group]
    }
    for key in floors:
        label, native = key.split("/", 1)
        if label in groups and native in groups:
            found.add((label, native))

    return found


def ratios(results, floors):
    """The geometric mean of each pair's IOPS ratio, the pairs, and the unpaired
    xNVMe engines"""

    groups, points = {}, defaultdict(lambda: defaultdict(list))
    for _, metrics in results:
        ctx = metrics["ctx"]
        groups[ctx["label"]] = ctx["group"]
        point = (ctx.get("iosize", ctx.get("bs")), ctx["iodepth"])
        points[point][ctx["label"]].append(metrics["iops"])

    found = pairs(groups, floors)
    logs = defaultdict(list)
    for samples in points.values():
        iops = {label: sum(values) / len(values) for label, values in samples.items()}
        for label, native in found:
            if iops.get(label, 0) > 0 and iops.get(native, 0) > 0:
                logs[(label, native)].append(math.log(iops[label] / iops[native]))

    paired = {label for label, _ in found}
    unpaired = sorted(
        label for label in groups if is_xnvme(label) and label not in paired
    )

    return (
        {pair: math.exp(sum(values) / len(values)) for pair, values in logs.items()},
        found,
        unpaired,
    )


def idle(results):
    """The labels that reported no IOPS at some point"""

    return sorted({m["ctx"]["label"] for _, m in results if not m["iops"] > 0})


def check(results, floors):
    """What fails the run, as messages; none when it passes"""

    means, found, unpaired = ratios(results, floors)
    idled = idle(results)
    problems = [f"{label}: no IOPS; its benchmark failed" for label in idled]
    problems += [
        f"{label}: no native peer; name one in its floor" for label in unpaired
    ]
    problems += [
        f"{label}/{native}: no point where both report IOPS"
        for label, native in sorted(found - means.keys())
        if label not in idled and native not in idled
    ]
    for (label, native), ratio in sorted(means.items()):
        floor = floors.get(f"{label}/{native}")
        if floor is None:
            problems.append(f"{label}/{native}: {ratio:.3f}, and no floor")
        elif ratio < floor:
            problems.append(f"{label}/{native}: {ratio:.3f} < {floor}")
        else:
            log.info(f"{label}/{native}: {ratio:.3f} >= {floor}")

    return problems


def main(args, cijoe):
    """Check the normalized results under 'path', or the output, against floors"""

    paths = list(Path(args.path or args.output).rglob(OUTPUT_NORMALIZED_FILENAME))
    if not paths:
        log.error(f"no {OUTPUT_NORMALIZED_FILENAME}; run benchmark_normalize first")
        return errno.ENOENT

    os_name = cijoe.getconf("os.name", "")
    floors = (yaml.safe_load(args.thresholds.read_text()) or {}).get(os_name, {})

    problems = []
    for path in paths:
        problems += check(json.loads(path.read_text()), floors)
    for problem in problems:
        log.error(problem)

    return errno.EINVAL if problems else 0
