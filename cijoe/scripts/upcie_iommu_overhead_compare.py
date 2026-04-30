#!/usr/bin/env python3
"""
Compare normalized uPCIe IOMMU overhead outputs
===============================================
"""
import csv
import errno
import json
import logging as log
import traceback
from argparse import ArgumentParser
from itertools import product
from pathlib import Path

from cijoe.core.resources import dict_from_yamlfile

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"
COMPARISON_JSON = "upcie-iommu-overhead-comparison.json"
COMPARISON_CSV = "upcie-iommu-overhead-comparison.csv"
JSON_DUMP = {"indent": 4}


def add_args(parser: ArgumentParser):
    parser.add_argument("--uio-output", "--uio_output", type=Path, default=None)
    parser.add_argument("--vfio-output", "--vfio_output", type=Path, default=None)
    parser.add_argument(
        "--runs",
        type=Path,
        default=None,
        help="Path to a yaml file describing the workload matrix to compare",
    )


def load_normalized(output_path):
    path = Path(output_path) / "artifacts" / OUTPUT_NORMALIZED_FILENAME
    if not path.exists():
        path = Path(output_path) / OUTPUT_NORMALIZED_FILENAME
    with path.open() as jfd:
        return json.load(jfd)


def index_by_workload(data):
    indexed = {}
    for _, metrics in data:
        ctx = metrics["ctx"]
        key = (ctx["rw"], int(ctx["iosize"]), int(ctx["iodepth"]))
        indexed[key] = metrics
    return indexed


def workload_keys(runs_path):
    if not runs_path:
        return None

    keys = set()
    runs = dict_from_yamlfile(runs_path)
    for workload in runs.get("workloads", []):
        rw = workload["pattern"]
        for iosize, iodepth in product(workload["iosizes"], workload["iodepths"]):
            keys.add((rw, int(iosize), int(iodepth)))
    return keys


def pct_delta(base, value):
    if not base:
        log.warning(f"pct_delta baseline is zero (value={value})")
        return float("nan")

    return (value - base) / base * 100.0


def compare(args, cijoe):
    uio_output = args.uio_output or cijoe.getconf(
        "upcie_iommu_overhead.compare.uio_output", None
    )
    vfio_output = args.vfio_output or cijoe.getconf(
        "upcie_iommu_overhead.compare.vfio_output", None
    )

    if not uio_output or not vfio_output:
        log.error("Missing --uio-output and --vfio-output")
        return errno.EINVAL

    uio = index_by_workload(load_normalized(uio_output))
    vfio = index_by_workload(load_normalized(vfio_output))
    allowed_keys = workload_keys(args.runs)
    common_keys = set(uio) & set(vfio)
    if allowed_keys is not None:
        common_keys &= allowed_keys

    items = []
    for key in sorted(common_keys, key=lambda item: (item[0], item[1], item[2])):
        rw, iosize, iodepth = key
        uio_metrics = uio[key]
        vfio_metrics = vfio[key]

        items.append(
            {
                "ctx": {
                    "rw": rw,
                    "iosize": iosize,
                    "iodepth": iodepth,
                    "group": "upcie-iommu-overhead",
                },
                "uio": {
                    "driver": uio_metrics["ctx"].get("driver", "uio_pci_generic"),
                    "iops": uio_metrics["iops"],
                    "bwps": uio_metrics["bwps"],
                    "repeat": uio_metrics["ctx"].get("repeat", 1),
                    "iops_stddev": uio_metrics.get("stddev", 0),
                    "iops_cv": uio_metrics.get("iops_cv", 0),
                },
                "vfio": {
                    "driver": vfio_metrics["ctx"].get("driver", "vfio-pci"),
                    "iops": vfio_metrics["iops"],
                    "bwps": vfio_metrics["bwps"],
                    "repeat": vfio_metrics["ctx"].get("repeat", 1),
                    "iops_stddev": vfio_metrics.get("stddev", 0),
                    "iops_cv": vfio_metrics.get("iops_cv", 0),
                },
                "delta_pct": pct_delta(uio_metrics["iops"], vfio_metrics["iops"]),
            }
        )

    if not items:
        log.error("No matching workloads found between UIO and VFIO outputs")
        return errno.ENOENT

    artifacts = Path(args.output) / "artifacts"
    artifacts.mkdir(parents=True, exist_ok=True)

    payload = {
        "uio_output": str(uio_output),
        "vfio_output": str(vfio_output),
        "items": items,
    }
    with (artifacts / COMPARISON_JSON).open("w") as jfd:
        json.dump(payload, jfd, **JSON_DUMP)

    with (artifacts / COMPARISON_CSV).open("w", newline="") as cfd:
        fieldnames = [
            "rw",
            "iosize",
            "iodepth",
            "uio_iops",
            "vfio_iops",
            "delta_pct",
            "uio_cv",
            "vfio_cv",
        ]
        writer = csv.DictWriter(cfd, fieldnames=fieldnames)
        writer.writeheader()
        for item in items:
            writer.writerow(
                {
                    "rw": item["ctx"]["rw"],
                    "iosize": item["ctx"]["iosize"],
                    "iodepth": item["ctx"]["iodepth"],
                    "uio_iops": f"{item['uio']['iops']:.6f}",
                    "vfio_iops": f"{item['vfio']['iops']:.6f}",
                    "delta_pct": f"{item['delta_pct']:.6f}",
                    "uio_cv": f"{item['uio']['iops_cv']:.6f}",
                    "vfio_cv": f"{item['vfio']['iops_cv']:.6f}",
                }
            )

    return 0


def main(args, cijoe):
    try:
        return compare(args, cijoe)
    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        return 1
