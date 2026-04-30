#!/usr/bin/env python3
"""
Normalize xnvmeperf uPCIe IOMMU overhead outputs
================================================
"""
import errno
import hashlib
import json
import logging as log
import re
import statistics
import traceback
from argparse import ArgumentParser
from collections import OrderedDict
from pathlib import Path

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"
JSON_DUMP = {"indent": 4}
GROUP = "upcie-iommu-overhead"

STEM_REGEX = re.compile(
    r"xnvmeperf-output_IOSIZE=(?P<iosize>\d+)_IODEPTH=(?P<iodepth>\d+)_"
    r"LABEL=(?P<label>.+)_GROUP=(?P<group>.+)_RW=(?P<rw>.+)_"
    r"REP=(?P<rep>\d+)"
)
ELAPSED_REGEX = re.compile(r"xnvmeperf \(elapsed:\s*(?P<elapsed>\d+(?:\.\d+)?)s\)")
TOTAL_REGEX = re.compile(
    r"^\s*Total\s*:\s+(?P<iops>\d+(?:\.\d+)?)\s+"
    r"(?P<mibps>\d+(?:\.\d+)?)\s+(?P<failed>\d+(?:\.\d+)?)",
    re.MULTILINE,
)


def add_args(parser: ArgumentParser):
    parser.add_argument("--path", type=Path, default=None)


def parse_xnvmeperf_output(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    elapsed = ELAPSED_REGEX.search(text)
    total = TOTAL_REGEX.search(text)

    if not total:
        raise ValueError(f"failed to parse {path}")

    return {
        "elapsed": float(elapsed.group("elapsed")) if elapsed else 0.0,
        "iops": float(total.group("iops")),
        "mibps": float(total.group("mibps")),
        "failed": float(total.group("failed")),
    }


def mean(values):
    return sum(values) / len(values)


def stdev(values):
    return statistics.stdev(values) if len(values) > 1 else 0.0


def cv(values):
    avg = mean(values)
    return stdev(values) / avg * 100.0 if avg else 0.0


def stable_ident(context):
    digest = hashlib.md5()
    digest.update(
        "".join(
            f"{key}={value}"
            for key, value in sorted(context.items())
            if key not in ["repeat"]
        ).encode()
    )
    return digest.hexdigest()


def normalize(args):
    search = Path(args.path or args.output)

    groups = OrderedDict()

    for path in sorted(search.rglob("xnvmeperf-output_*.txt")):
        match = STEM_REGEX.match(path.stem)
        if not match:
            continue

        parsed = parse_xnvmeperf_output(path)
        key = (
            match.group("label"),
            match.group("group"),
            match.group("rw"),
            int(match.group("iosize")),
            int(match.group("iodepth")),
        )
        groups.setdefault(key, []).append(parsed)

    if not groups:
        log.error("No xnvmeperf-output_*.txt files found")
        return errno.ENOENT

    collection = []
    for (label, group, rw, iosize, iodepth), samples in groups.items():
        iops = [sample["iops"] for sample in samples]
        mibps = [sample["mibps"] for sample in samples]
        elapsed = [sample["elapsed"] for sample in samples]
        failed = [sample["failed"] for sample in samples]

        context = {
            "name": label,
            "label": label,
            "group": group,
            "ioengine": "xnvmeperf",
            "rw": rw,
            "iosize": iosize,
            "iodepth": iodepth,
            "driver": label,
            "xnvme_be": "upcie",
            "repeat": len(samples),
        }

        metrics = {
            "ctx": context,
            "iops": mean(iops),
            "bwps": mean(mibps),
            "lat": 0,
            "stddev": stdev(iops),
            "cpu": 0,
            "elapsed": mean(elapsed),
            "failed": sum(failed),
            "iops_cv": cv(iops),
        }
        collection.append((stable_ident(context), metrics))

    artifacts = Path(args.output) / "artifacts"
    artifacts.mkdir(parents=True, exist_ok=True)
    with (artifacts / OUTPUT_NORMALIZED_FILENAME).open("w") as jfd:
        json.dump(collection, jfd, **JSON_DUMP)

    return 0


def main(args, cijoe):
    try:
        return normalize(args)
    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        return 1
