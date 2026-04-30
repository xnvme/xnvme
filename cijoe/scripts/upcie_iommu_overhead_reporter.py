#!/usr/bin/env python3
"""
Produce a uPCIe IOMMU overhead report
=====================================
"""
import errno
import json
import logging as log
import re
import shlex
import shutil
import traceback
from argparse import ArgumentParser
from collections import defaultdict
from pathlib import Path

import jinja2
from reporter import (
    copy_graphs,
    create_stylesheet,
    create_test_setup,
    create_xnvme_cover,
    create_xnvme_info,
)

from cijoe.core.resources import dict_from_yamlfile

COMPARISON_JSON = "upcie-iommu-overhead-comparison.json"
PLOT_PATH_REGEX = r".*_RW=(?P<rw>.+)_IOSIZE=(?P<iosize>\d+)_TYPE=(?P<type>.+)\.png"
TAIL_LATENCIES = [
    ("p99_9", "P99.9"),
    ("p99_99", "P99.99"),
    ("p99_999", "P99.999"),
]


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--templates", type=Path, default=Path.cwd() / "templates" / "perf_report"
    )
    parser.add_argument("--path", type=Path, default=None)
    parser.add_argument(
        "--runs",
        type=Path,
        default=None,
        help="Path to the workload matrix used by the compared benchmark runs",
    )
    parser.add_argument("--report_title", type=str, default="xNVMe uPCIe")
    parser.add_argument("--report_subtitle", type=str, default="IOMMU Overhead Report")


def load_comparison(search):
    path = Path(search) / "artifacts" / COMPARISON_JSON
    if not path.exists():
        path = Path(search) / COMPARISON_JSON
    with path.open() as jfd:
        return json.load(jfd)


def format_items(items):
    rows = []
    for item in items:
        row = {
            "rw": item["ctx"]["rw"],
            "iosize": item["ctx"]["iosize"],
            "iodepth": item["ctx"]["iodepth"],
            "throughput_runner": "xnvmeperf",
            "latency_runner": "fio",
            "uio_iops": f"{item['uio']['iops']:.2f}",
            "vfio_iops": f"{item['vfio']['iops']:.2f}",
            "delta_pct": f"{item['delta_pct']:.2f}",
            "uio_fio_iops": f"{item['uio']['fio_iops']:.2f}",
            "vfio_fio_iops": f"{item['vfio']['fio_iops']:.2f}",
            "fio_delta_pct": f"{item['fio_delta_pct']:.2f}",
            "uio_fio_cv": f"{item['uio']['fio_iops_cv']:.2f}",
            "vfio_fio_cv": f"{item['vfio']['fio_iops_cv']:.2f}",
            "uio_lat_us": f"{item['uio']['lat_ns'] / 1000.0:.2f}",
            "vfio_lat_us": f"{item['vfio']['lat_ns'] / 1000.0:.2f}",
            "lat_delta_pct": f"{item['lat_delta_pct']:.2f}",
            "uio_cv": f"{item['uio']['iops_cv']:.2f}",
            "vfio_cv": f"{item['vfio']['iops_cv']:.2f}",
        }
        for percentile, _ in TAIL_LATENCIES:
            row[f"uio_{percentile}_us"] = (
                f"{item['uio']['tail_lat_ns'][percentile] / 1000.0:.2f}"
            )
            row[f"vfio_{percentile}_us"] = (
                f"{item['vfio']['tail_lat_ns'][percentile] / 1000.0:.2f}"
            )
            row[f"{percentile}_delta_pct"] = (
                f"{item['tail_lat_delta_pct'][percentile]:.2f}"
            )
        rows.append(row)
    return rows


def plot_groups(artifacts):
    groups = defaultdict(dict)
    for path in artifacts.glob("upcie_iommu_overhead_RW=*_IOSIZE=*_TYPE=*.png"):
        match = re.match(PLOT_PATH_REGEX, path.name)
        if not match:
            continue
        key = (match.group("rw"), match.group("iosize"))
        groups[key][match.group("type")] = path.name
    return groups


def workload_matrix(runs_path):
    if not runs_path:
        return []

    runs = dict_from_yamlfile(runs_path)
    matrix = []
    for workload in runs.get("workloads", []):
        matrix.append(
            {
                "rw": workload["pattern"],
                "iosizes": ", ".join(str(value) for value in workload["iosizes"]),
                "iodepths": ", ".join(str(value) for value in workload["iodepths"]),
            }
        )
    return matrix


def report_sections(plots, rows):
    grouped_rows = defaultdict(list)
    for row in rows:
        grouped_rows[(row["rw"], str(row["iosize"]))].append(row)

    sections = []
    for key, group_plots in sorted(plots.items(), key=lambda item: item[0]):
        group_rows = sorted(
            grouped_rows.get(key, []),
            key=lambda row: int(row["iodepth"]),
        )
        sections.append(
            {
                "rw": key[0],
                "iosize": key[1],
                "plots": group_plots,
                "rows": group_rows,
            }
        )
    return sections


def main(args, cijoe):
    try:
        templates_path = args.templates.resolve()
        search_path = Path(args.path or cijoe.output_path).resolve()
        artifacts = search_path / "artifacts"

        report_path = cijoe.output_path / "artifacts" / "perf_report"
        if report_path.exists():
            shutil.rmtree(report_path)
        report_path.mkdir(parents=False, exist_ok=True)

        body_path = report_path / "report.rst"
        pdf_path = report_path / "upcie-iommu-overhead.pdf"
        style_path = create_stylesheet(templates_path, report_path)
        cover_path = create_xnvme_cover(
            templates_path, report_path, args.report_title, args.report_subtitle
        )
        create_xnvme_info(templates_path, report_path)
        create_test_setup(templates_path, report_path, artifacts)
        copy_graphs(report_path, artifacts)

        comparison = load_comparison(search_path)
        rows = format_items(comparison["items"])
        plots = plot_groups(artifacts)
        workloads = workload_matrix(args.runs)
        sections = report_sections(plots, rows)

        template_loader = jinja2.FileSystemLoader(templates_path)
        template_env = jinja2.Environment(loader=template_loader)
        template = template_env.get_template("upcie_iommu_overhead.jinja2.rst")

        with body_path.open("w") as body:
            body.write(
                template.render(
                    {
                        "title": args.report_title,
                        "subtitle": args.report_subtitle,
                        "comparison": comparison,
                        "rows": rows,
                        "plots": plots,
                        "workloads": workloads,
                        "sections": sections,
                        "tail_latencies": TAIL_LATENCIES,
                    }
                )
            )

        err, _ = cijoe.run_local(
            f"rst2pdf {shlex.quote(str(body_path))}"
            f" -b1"
            f" --custom-cover {shlex.quote(str(cover_path))}"
            f" -s {shlex.quote(str(style_path))}"
            f" -o {shlex.quote(str(pdf_path))}"
        )
        return err
    except StopIteration:
        log.error("Missing comparison artifact")
        return errno.ENOENT
    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        return 1
