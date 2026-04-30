#!/usr/bin/env python3
"""
Plot uPCIe IOMMU overhead comparisons
=====================================
"""
import errno
import json
import logging as log
import os
import traceback
from argparse import ArgumentParser
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

COMPARISON_JSON = "upcie-iommu-overhead-comparison.json"


def add_args(parser: ArgumentParser):
    parser.add_argument("--path", type=Path, default=None)


def safe_name(value):
    return str(value).replace("/", "-")


def grouped_items(items):
    grouped = defaultdict(list)
    for item in items:
        ctx = item["ctx"]
        grouped[(ctx["rw"], ctx["iosize"])].append(item)
    for entries in grouped.values():
        entries.sort(key=lambda item: item["ctx"]["iodepth"])
    return grouped


def load_comparison(search):
    path = Path(search) / "artifacts" / COMPARISON_JSON
    if not path.exists():
        path = Path(search) / COMPARISON_JSON
    with path.open() as jfd:
        return json.load(jfd)


def plot_iops(entries, output_path):
    qdepths = [item["ctx"]["iodepth"] for item in entries]
    uio = [item["uio"]["iops"] for item in entries]
    vfio = [item["vfio"]["iops"] for item in entries]
    xs = range(len(qdepths))
    width = 0.35

    plt.clf()
    fig, ax = plt.subplots()
    ax.bar([x - width / 2 for x in xs], uio, width, label="uio_pci_generic")
    ax.bar([x + width / 2 for x in xs], vfio, width, label="vfio-pci")
    ax.set_xlabel("iodepth")
    ax.set_ylabel("IOPS")
    ax.set_xticks(list(xs), [str(qd) for qd in qdepths])
    ax.legend()
    fig.tight_layout()
    plt.savefig(output_path)


def plot_delta(entries, output_path):
    qdepths = [item["ctx"]["iodepth"] for item in entries]
    delta = [item["delta_pct"] for item in entries]
    xs = range(len(qdepths))

    plt.clf()
    fig, ax = plt.subplots()
    ax.bar(xs, delta, 0.5, color="#2B7B9B")
    ax.axhline(0, color="black", linewidth=0.8)
    ax.set_xlabel("iodepth")
    ax.set_ylabel("VFIO vs UIO IOPS delta (%)")
    ax.set_xticks(list(xs), [str(qd) for qd in qdepths])
    fig.tight_layout()
    plt.savefig(output_path)


def create_plots(args):
    search = args.path or args.output
    if not search:
        return errno.EINVAL

    artifacts = args.output / "artifacts"
    os.makedirs(artifacts, exist_ok=True)

    comparison = load_comparison(search)
    for (rw, iosize), entries in grouped_items(comparison["items"]).items():
        prefix = f"upcie_iommu_overhead_RW={safe_name(rw)}_IOSIZE={iosize}"
        plot_iops(entries, artifacts / f"{prefix}_TYPE=iops.png")
        plot_delta(entries, artifacts / f"{prefix}_TYPE=delta.png")

    return 0


def main(args, cijoe):
    try:
        return create_plots(args)
    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        return 1
