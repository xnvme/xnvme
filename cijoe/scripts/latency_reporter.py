#!/usr/bin/env python3
"""
Produce a performance report
============================

Create it for the current results or overwrite via step-args.

Step Args
---------

step.with.xnvme_source:  path to xNVMe source (default: config.options.repository.path)

Retargetable: True
------------------
"""
import errno
from datetime import date
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
    pdf_path = report_path / "latency.pdf"
    logo_path = report_path / "xnvme.png"

    # Can be from a different run / results
    plot_path = (
        Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
        / "artifacts"
    )

    report_path.mkdir(parents=False, exist_ok=False)

    # Fill 'report_path' with files needed to produce the .pdf
    copyfile(templates_path / "style.yaml", style_path, follow_symlinks=False)
    copyfile(templates_path / "xnvme.png", logo_path, follow_symlinks=False)

    # Copy graphs from results/artifacts into 'report_path'
    for png_path in plot_path.glob("*.png"):
        print(png_path)
        copyfile(png_path, report_path / png_path.name)

    # Read the cover-template, populate it, then store it in the artifacts directory
    with (templates_path / "cover.jinja2.tmpl").open() as template:
        with cover_path.open("w") as cover:
            cover.write(
                template.read().replace(
                    "INSERT_DATE", date.today().strftime("%d %B %Y")
                )
            )

    # Read the report-template, populate it, then store it in the artifacts directory
    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)
    template = template_env.get_template("latency.jinja2.rst")
    with body_path.open("w") as body:
        plots = [
            {"title": p.name[13:-4], "png": p.name} for p in plot_path.glob("*.png")
        ]
        tmpl = template.render({"plots": plots})
        body.write(tmpl)

    err, _ = cijoe.run_local(
        f"rst2pdf {body_path}"
        f" -b1"
        f" --custom-cover {cover_path}"
        f" -s {style_path}"
        f" -o {pdf_path}"
    )

    return err
