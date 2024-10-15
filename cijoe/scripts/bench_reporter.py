#!/usr/bin/env python3
"""
Produce a report
============================

Create it for the current results or overwrite via step-args.

Step Args
---------

step.with.xnvme_source:  path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
from pathlib import Path

import jinja2
from reporter import (
    copy_graphs,
    create_stylesheet,
    create_test_setup,
    create_xnvme_cover,
    create_xnvme_info,
)


def main(args, cijoe, step):
    """Primary entry-point"""
    templates_path = Path(
        step.get("with", {}).get("templates", Path.cwd() / "templates" / "perf_report")
    ).resolve()
    search_path = Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
    artifacts = search_path / "artifacts"

    title = step.get("with", {}).get("report_title", "xNVMe")
    subtitle = step.get("with", {}).get("report_subtitle", "Benchmark Report")

    report_path = cijoe.output_path / "artifacts" / "perf_report"
    report_path.mkdir(parents=False, exist_ok=True)

    # Files emitted by this script
    body_path = report_path / "report.rst"
    pdf_path = report_path / "perf.pdf"
    style_path = create_stylesheet(templates_path, report_path)
    cover_path = create_xnvme_cover(templates_path, report_path, title, subtitle)
    create_xnvme_info(templates_path, report_path)
    create_test_setup(templates_path, report_path, artifacts)

    copy_graphs(report_path, artifacts)

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

    # Read the report-template, populate it, then store it in the artifacts directory
    template = template_env.get_template("bench.jinja2.rst")
    with body_path.open("w") as body:
        body.write(
            template.render(
                {
                    "title": title,
                    "subtitle": subtitle,
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
