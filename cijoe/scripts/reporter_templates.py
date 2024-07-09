#!/usr/bin/env python3
"""
Report templates
============================

Create the general templates for the xNVMe reports.

"""
import errno
import json
import logging as log
import re
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
    xnvme_path = report_path / "xnvme.rst"
    testsetup_path = report_path / "testsetup.rst"
    logo_path = report_path / "xnvme.png"

    # Can be from a different run / results
    prefix_path = Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
    sysinfo_path = prefix_path / "artifacts" / "sysinfo.json"
    biosinfo_path = prefix_path / "artifacts" / "biosinfo.json"

    report_path.mkdir(parents=False, exist_ok=True)

    # Fill 'report_path' with files needed to produce the .pdf
    copyfile(templates_path / "xnvme.png", logo_path, follow_symlinks=False)

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

    # Read the cover-template, populate it, then store it in the artifacts directory
    title = step.get("with", {}).get("report_title", "xNVMe")
    subtitle = step.get("with", {}).get("report_subtitle", "Report")
    template = template_env.get_template("cover.jinja2.rst")
    with cover_path.open("w") as cover:
        today = date.today().strftime("%d %B %Y")
        tmpl = template.render({"title": title, "subtitle": subtitle, "date": today})
        cover.write(tmpl)

    # Read the template with the description of xNVMe, then store it in the artifacts directory
    template = template_env.get_template("xnvme.jinja2.rst")
    with xnvme_path.open("w") as xnvme_desc:
        tmpl = template.render()
        xnvme_desc.write(tmpl)

    # Read the template with the test setup, then store it in the artifacts directory
    sysinfo = json.load(open(sysinfo_path))
    biosinfo = json.load(open(biosinfo_path))
    template = template_env.get_template("testsetup.jinja2.rst")
    with testsetup_path.open("w") as testsetup:
        tmpl = template.render(
            {
                "sysinfo": {
                    "Operating system": sysinfo["operating_system"],
                    "Kernel": sysinfo["kernel"],
                    "Motherboard": sysinfo["motherboard"],
                    "CPU": sysinfo["cpu"],
                    "Memory": sysinfo["memory"],
                    "Drives under test": sysinfo["drives"],
                },
                "biosinfo": biosinfo,
            }
        )
        testsetup.write(tmpl)

    return 0
