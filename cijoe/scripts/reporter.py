"""
Report templates
================

Helper functions for creating xNVMe reports

"""

import json
from datetime import date
from pathlib import Path
from shutil import copyfile

import jinja2


def copy_graphs(report_path: Path, artifacts: Path):
    # Copy graphs from results/artifacts into 'report_path'
    for png_path in artifacts.glob("*.png"):
        print(png_path)
        copyfile(png_path, report_path / png_path.name)


def create_stylesheet(templates_path: Path, report_path: Path):
    style_path = report_path / "style.yaml"
    copyfile(templates_path / "style.yaml", style_path, follow_symlinks=False)

    return style_path


def create_xnvme_cover(
    templates_path: Path, report_path: Path, title: str, subtitle: str
):
    cover_path = report_path / "cover.tmpl"
    logo_path = report_path / "xnvme.png"

    # Copy the template logo to the 'report_path'
    copyfile(templates_path / "xnvme.png", logo_path, follow_symlinks=False)

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

    # Read the cover-template, populate it, then store it in the artifacts directory
    template = template_env.get_template("cover.jinja2.rst")
    with cover_path.open("w") as cover:
        today = date.today().strftime("%d %B %Y")
        tmpl = template.render({"title": title, "subtitle": subtitle, "date": today})
        cover.write(tmpl)

    return cover_path


def create_xnvme_info(templates_path: Path, report_path: Path):
    xnvme_path = report_path / "xnvme.rst"

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

    # Read the template with the description of xNVMe, then store it in the artifacts directory
    template = template_env.get_template("xnvme.jinja2.rst")
    with xnvme_path.open("w") as xnvme_desc:
        tmpl = template.render()
        xnvme_desc.write(tmpl)

    return xnvme_path


def create_test_setup(templates_path: Path, report_path: Path, artifacts: Path):
    testsetup_path = report_path / "testsetup.rst"

    sysinfo_path = artifacts / "sysinfo.json"
    biosinfo_path = artifacts / "biosinfo.json"

    template_loader = jinja2.FileSystemLoader(templates_path)
    template_env = jinja2.Environment(loader=template_loader)

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

    return testsetup_path
