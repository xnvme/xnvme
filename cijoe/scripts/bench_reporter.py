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
from shutil import copyfile

import jinja2


def main(args, cijoe, step):
    """Primary entry-point"""

    templates_path = Path(
        step.get("with", {}).get("templates", Path.cwd() / "templates" / "perf_report")
    ).resolve()

    # Files emitted by this script
    report_path = cijoe.output_path / "artifacts" / "perf_report"
    cover_path = report_path / "cover.tmpl"
    body_path = report_path / "report.rst"
    style_path = report_path / "style.yaml"
    pdf_path = report_path / "perf.pdf"

    # Can be from a different run / results
    prefix_path = Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
    plot_path = prefix_path / "artifacts"

    report_path.mkdir(parents=False, exist_ok=True)

    # Fill 'report_path' with files needed to produce the .pdf
    copyfile(templates_path / "style.yaml", style_path, follow_symlinks=False)

    # Copy graphs from results/artifacts into 'report_path'
    for png_path in plot_path.glob("*.png"):
        print(png_path)
        copyfile(png_path, report_path / png_path.name)

    title = step.get("with", {}).get("report_title", "xNVMe")
    subtitle = step.get("with", {}).get("report_subtitle", "Report")

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
