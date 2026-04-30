#!/usr/bin/env python3
"""
Normalize uPCIe IOMMU overhead outputs
======================================
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
TAIL_LATENCIES = {
    "p99_9": "99.900000",
    "p99_99": "99.990000",
    "p99_999": "99.999000",
}

STEM_REGEX = re.compile(
    r"xnvmeperf-output_IOSIZE=(?P<iosize>\d+)_IODEPTH=(?P<iodepth>\d+)_"
    r"LABEL=(?P<label>.+)_GROUP=(?P<group>.+)_RW=(?P<rw>.+)_"
    r"REP=(?P<rep>\d+)"
)
FIO_STEM_REGEX = re.compile(
    r"fio-output_IOSIZE=(?P<iosize>\d+)_IODEPTH=(?P<iodepth>\d+)_"
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


def parse_fio_output(path, rw):
    data = json.loads(path.read_text(encoding="utf-8"))
    if not data.get("jobs"):
        raise ValueError(f"fio output has no jobs: {path}")

    kind = "read" if "read" in rw else "write"
    job = data["jobs"][0]
    section = job.get(kind, {})
    lat_ns = section.get("lat_ns", {})
    clat_ns = section.get("clat_ns", {})
    percentile = clat_ns.get("percentile", lat_ns.get("percentile", {}))

    if not lat_ns:
        raise ValueError(f"fio output has no {kind}.lat_ns: {path}")

    tail_lat_ns = {
        name: float(percentile.get(fio_key, 0))
        for name, fio_key in TAIL_LATENCIES.items()
    }

    return {
        "runtime_ms": float(job.get("job_runtime", 0)),
        "iops": float(section.get("iops", 0.0)),
        "bwps": float(section.get("bw_bytes", 0)),
        "lat_ns": float(lat_ns.get("mean", 0.0)),
        "lat_stddev_ns": float(lat_ns.get("stddev", 0.0)),
        "clat_ns": float(clat_ns.get("mean", lat_ns.get("mean", 0.0))),
        "clat_p99_ns": float(percentile.get("99.000000", percentile.get("99.00", 0))),
        "tail_lat_ns": tail_lat_ns,
        "error": float(job.get("error", 0)),
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

    xnvmeperf_groups = OrderedDict()

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
        xnvmeperf_groups.setdefault(key, []).append(parsed)

    if not xnvmeperf_groups:
        log.error("No xnvmeperf-output_*.txt files found")
        return errno.ENOENT

    fio_groups = OrderedDict()
    for path in sorted(search.rglob("fio-output_*.txt")):
        match = FIO_STEM_REGEX.match(path.stem)
        if not match:
            continue

        key = (
            match.group("label"),
            match.group("group"),
            match.group("rw"),
            int(match.group("iosize")),
            int(match.group("iodepth")),
        )
        parsed = parse_fio_output(path, match.group("rw"))
        fio_groups.setdefault(key, []).append(parsed)

    collection = []
    for key, xnvmeperf_samples in xnvmeperf_groups.items():
        label, group, rw, iosize, iodepth = key
        fio_samples = fio_groups.get(key)
        if not fio_samples:
            raise ValueError(f"missing fio latency samples for {key}")

        iops = [sample["iops"] for sample in xnvmeperf_samples]
        mibps = [sample["mibps"] for sample in xnvmeperf_samples]
        elapsed = [sample["elapsed"] for sample in xnvmeperf_samples]
        failed = [sample["failed"] for sample in xnvmeperf_samples]
        lat = [sample["lat_ns"] for sample in fio_samples]
        tail_lat = {
            name: [sample["tail_lat_ns"][name] for sample in fio_samples]
            for name in TAIL_LATENCIES
        }
        fio_iops = [sample["iops"] for sample in fio_samples]
        fio_bwps = [sample["bwps"] for sample in fio_samples]
        fio_errors = [sample["error"] for sample in fio_samples]

        context = {
            "name": label,
            "label": label,
            "group": group,
            "ioengine": "xnvmeperf+fio",
            "rw": rw,
            "iosize": iosize,
            "iodepth": iodepth,
            "driver": label,
            "xnvme_be": "upcie",
            "repeat": len(xnvmeperf_samples),
            "throughput_source": "fio.iops",
            "latency_source": "fio.lat_ns.mean",
            "tail_latency_source": "fio.clat_ns.percentile",
        }

        metrics = {
            "ctx": context,
            "iops": mean(iops),
            "bwps": mean(mibps),
            "lat": mean(lat),
            "stddev": stdev(iops),
            "lat_stddev": stdev(lat),
            "tail_lat_ns": {name: mean(values) for name, values in tail_lat.items()},
            "tail_lat_stddev_ns": {
                name: stdev(values) for name, values in tail_lat.items()
            },
            "tail_lat_cv": {name: cv(values) for name, values in tail_lat.items()},
            "cpu": 0,
            "elapsed": mean(elapsed),
            "failed": sum(failed),
            "iops_cv": cv(iops),
            "lat_cv": cv(lat),
            "fio_iops": mean(fio_iops),
            "fio_iops_stddev": stdev(fio_iops),
            "fio_iops_cv": cv(fio_iops),
            "fio_bwps": mean(fio_bwps),
            "fio_error": sum(fio_errors),
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
