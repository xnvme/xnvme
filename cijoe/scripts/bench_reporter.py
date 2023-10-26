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
from pathlib import Path
from shutil import copyfile


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
            cover.write(template.read().replace("INSERT_DATE", "1st of January 2024"))

    # Read the report-template, populate it, then store it in the artifacts directory
    with (templates_path / "bench.jinja2.rst").open() as template:
        with body_path.open("w") as body:
            body.write(template.read())

    err, _ = cijoe.run_local(
        f"rst2pdf {body_path}"
        f" -b1"
        f" --custom-cover {cover_path}"
        f" -s {style_path}"
        f" -o {pdf_path}"
    )

    return err
