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
TAIL_LATENCIES = ["p99_9", "p99_99", "p99_999"]


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


def metric_latency_ns(metrics):
    ctx = metrics.get("ctx", {})
    if ctx.get("latency_source") != "fio.lat_ns.mean":
        raise ValueError(f"missing fio latency source for {ctx}")

    lat = float(metrics.get("lat", 0))
    if lat <= 0:
        raise ValueError(f"missing fio latency for {ctx}")
    return lat


def metric_tail_latency_ns(metrics):
    ctx = metrics.get("ctx", {})
    if ctx.get("tail_latency_source") != "fio.clat_ns.percentile":
        raise ValueError(f"missing fio tail latency source for {ctx}")

    values = metrics.get("tail_lat_ns", {})
    missing = [name for name in TAIL_LATENCIES if float(values.get(name, 0)) <= 0]
    if missing:
        raise ValueError(f"missing fio tail latency {missing} for {ctx}")

    return {name: float(values[name]) for name in TAIL_LATENCIES}


def metric_fio_iops(metrics):
    ctx = metrics.get("ctx", {})
    if ctx.get("throughput_source") != "fio.iops":
        raise ValueError(f"missing fio metrics for {ctx}")

    iops = float(metrics.get("fio_iops", 0))
    if iops <= 0:
        raise ValueError(f"missing fio iops for {ctx}")
    return iops


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
        uio_lat_ns = metric_latency_ns(uio_metrics)
        vfio_lat_ns = metric_latency_ns(vfio_metrics)
        uio_tail_lat_ns = metric_tail_latency_ns(uio_metrics)
        vfio_tail_lat_ns = metric_tail_latency_ns(vfio_metrics)
        uio_fio_iops = metric_fio_iops(uio_metrics)
        vfio_fio_iops = metric_fio_iops(vfio_metrics)

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
                    "fio_iops": uio_fio_iops,
                    "fio_bwps": uio_metrics.get("fio_bwps", 0),
                    "fio_iops_stddev": uio_metrics.get("fio_iops_stddev", 0),
                    "fio_iops_cv": uio_metrics.get("fio_iops_cv", 0),
                    "lat_ns": uio_lat_ns,
                    "repeat": uio_metrics["ctx"].get("repeat", 1),
                    "iops_stddev": uio_metrics.get("stddev", 0),
                    "iops_cv": uio_metrics.get("iops_cv", 0),
                    "lat_stddev_ns": uio_metrics.get("lat_stddev", 0),
                    "lat_cv": uio_metrics.get("lat_cv", 0),
                    "tail_lat_ns": uio_tail_lat_ns,
                    "tail_lat_stddev_ns": uio_metrics.get("tail_lat_stddev_ns", {}),
                    "tail_lat_cv": uio_metrics.get("tail_lat_cv", {}),
                },
                "vfio": {
                    "driver": vfio_metrics["ctx"].get("driver", "vfio-pci"),
                    "iops": vfio_metrics["iops"],
                    "bwps": vfio_metrics["bwps"],
                    "fio_iops": vfio_fio_iops,
                    "fio_bwps": vfio_metrics.get("fio_bwps", 0),
                    "fio_iops_stddev": vfio_metrics.get("fio_iops_stddev", 0),
                    "fio_iops_cv": vfio_metrics.get("fio_iops_cv", 0),
                    "lat_ns": vfio_lat_ns,
                    "repeat": vfio_metrics["ctx"].get("repeat", 1),
                    "iops_stddev": vfio_metrics.get("stddev", 0),
                    "iops_cv": vfio_metrics.get("iops_cv", 0),
                    "lat_stddev_ns": vfio_metrics.get("lat_stddev", 0),
                    "lat_cv": vfio_metrics.get("lat_cv", 0),
                    "tail_lat_ns": vfio_tail_lat_ns,
                    "tail_lat_stddev_ns": vfio_metrics.get("tail_lat_stddev_ns", {}),
                    "tail_lat_cv": vfio_metrics.get("tail_lat_cv", {}),
                },
                "delta_pct": pct_delta(uio_metrics["iops"], vfio_metrics["iops"]),
                "fio_delta_pct": pct_delta(uio_fio_iops, vfio_fio_iops),
                "lat_delta_pct": pct_delta(uio_lat_ns, vfio_lat_ns),
                "tail_lat_delta_pct": {
                    name: pct_delta(uio_tail_lat_ns[name], vfio_tail_lat_ns[name])
                    for name in TAIL_LATENCIES
                },
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
            "uio_fio_iops",
            "vfio_fio_iops",
            "fio_delta_pct",
            "uio_lat_ns",
            "vfio_lat_ns",
            "lat_delta_pct",
            "uio_p99_9_ns",
            "vfio_p99_9_ns",
            "p99_9_delta_pct",
            "uio_p99_99_ns",
            "vfio_p99_99_ns",
            "p99_99_delta_pct",
            "uio_p99_999_ns",
            "vfio_p99_999_ns",
            "p99_999_delta_pct",
            "uio_cv",
            "vfio_cv",
            "uio_fio_cv",
            "vfio_fio_cv",
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
                    "uio_fio_iops": f"{item['uio']['fio_iops']:.6f}",
                    "vfio_fio_iops": f"{item['vfio']['fio_iops']:.6f}",
                    "fio_delta_pct": f"{item['fio_delta_pct']:.6f}",
                    "uio_lat_ns": f"{item['uio']['lat_ns']:.6f}",
                    "vfio_lat_ns": f"{item['vfio']['lat_ns']:.6f}",
                    "lat_delta_pct": f"{item['lat_delta_pct']:.6f}",
                    "uio_p99_9_ns": f"{item['uio']['tail_lat_ns']['p99_9']:.6f}",
                    "vfio_p99_9_ns": f"{item['vfio']['tail_lat_ns']['p99_9']:.6f}",
                    "p99_9_delta_pct": f"{item['tail_lat_delta_pct']['p99_9']:.6f}",
                    "uio_p99_99_ns": f"{item['uio']['tail_lat_ns']['p99_99']:.6f}",
                    "vfio_p99_99_ns": f"{item['vfio']['tail_lat_ns']['p99_99']:.6f}",
                    "p99_99_delta_pct": f"{item['tail_lat_delta_pct']['p99_99']:.6f}",
                    "uio_p99_999_ns": f"{item['uio']['tail_lat_ns']['p99_999']:.6f}",
                    "vfio_p99_999_ns": f"{item['vfio']['tail_lat_ns']['p99_999']:.6f}",
                    "p99_999_delta_pct": f"{item['tail_lat_delta_pct']['p99_999']:.6f}",
                    "uio_cv": f"{item['uio']['iops_cv']:.6f}",
                    "vfio_cv": f"{item['vfio']['iops_cv']:.6f}",
                    "uio_fio_cv": f"{item['uio']['fio_iops_cv']:.6f}",
                    "vfio_fio_cv": f"{item['vfio']['fio_iops_cv']:.6f}",
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
