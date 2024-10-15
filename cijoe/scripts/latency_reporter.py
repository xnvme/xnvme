#!/usr/bin/env python3
"""
Produce a latency report
============================

Create it for the current results or overwrite via step-args.

Step Args
---------

step.with.xnvme_source:  path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
import logging as log
import re
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

PLOT_PATH_REGEX = r".*_GROUP=(?P<group>.+)_TYPE=(?P<type>.+)\.png"


def main(args, cijoe, step):
    """Primary entry-point"""

    templates_path = Path(
        step.get("with", {}).get("templates", Path.cwd() / "templates" / "perf_report")
    ).resolve()
    search_path = Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
    artifacts = search_path / "artifacts"

    title = step.get("with", {}).get("report_title", "xNVMe")
    subtitle = step.get("with", {}).get("report_subtitle", "Latency Report")

    report_path = cijoe.output_path / "artifacts" / "perf_report"
    report_path.mkdir(parents=False, exist_ok=True)

    # Files emitted by this script
    body_path = report_path / "report.rst"
    pdf_path = report_path / "latency.pdf"
    style_path = create_stylesheet(templates_path, report_path)
    cover_path = create_xnvme_cover(templates_path, report_path, title, subtitle)
    create_xnvme_info(templates_path, report_path)
    create_test_setup(templates_path, report_path, artifacts)

    copy_graphs(report_path, artifacts)

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

    # info for methodology section
    ioengines = cijoe.config.options.get("fio", {}).get("engines", {})
    iosizes = step.get("with", {}).get("iosizes", [])
    iodepths = step.get("with", {}).get("iodepths", [])
    fio = {
        "iosizes": iosizes,
        "iodepths": iodepths,
        "ioengines": [{"name": e} for e in ioengines],
    }

    # plots for variable IO size and IO depths
    plot_paths = artifacts.glob("fio_*plot_scalability*.png")

    def path_to_object(path_path):
        match = re.match(PLOT_PATH_REGEX, path_path.name)
        return {
            "path": path_path.name,
            "group": match.group("group"),
            "type": match.group("type"),
        }

    plots = map(path_to_object, plot_paths)
    plot_groups = defaultdict(dict)
    for plot in plots:
        plot_groups[plot["group"]][plot["type"]] = plot["path"]

    # plot for IO depth 1
    qd1_pngs = list(artifacts.glob("fio_*qd1*.png"))
    if len(qd1_pngs) != 1:
        log.error(f"Unexpected paths: {qd1_pngs}")
        return errno.EINVAL
    qd1_plot = qd1_pngs[0].name

    # Read the report-template, populate it, then store it in the artifacts directory
    template = template_env.get_template("latency.jinja2.rst")
    with body_path.open("w") as body:
        body.write(
            template.render(
                {
                    "title": title,
                    "subtitle": subtitle,
                    "fio": fio,
                    "plots": plot_groups,
                    "qd1_plot": qd1_plot,
                }
            )
        )

    err, _ = cijoe.run_local(
        f"rst2pdf {body_path}"
        f" -b1"
        f" --custom-cover {cover_path}"
        f" -s {style_path}"
        f" -o {pdf_path}"
    )

    return err
