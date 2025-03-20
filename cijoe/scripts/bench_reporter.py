#!/usr/bin/env python3
"""
Produce a report
============================

Create it for the current results or overwrite via step-args.

Retargetable: True
------------------
"""
from argparse import ArgumentParser
from pathlib import Path

import jinja2
from reporter import (
    copy_graphs,
    create_stylesheet,
    create_test_setup,
    create_xnvme_cover,
    create_xnvme_info,
)


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--templates", type=Path, default=Path.cwd() / "templates" / "perf_report"
    )
    parser.add_argument("--path", type=str, default=None)
    parser.add_argument("--report_title", type=str, default="xNVMe")
    parser.add_argument("--report_subtitle", type=str, default="Benchmark Report")


def main(args, cijoe):
    """Primary entry-point"""
    templates_path = args.templates.resolve()
    search_path = Path(args.path or cijoe.output_path).resolve()
    artifacts = search_path / "artifacts"

    title = args.report_title
    subtitle = args.report_subtitle

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
