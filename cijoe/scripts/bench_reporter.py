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
from typing import Dict, Iterator, List, Tuple

import jinja2

PRODUCT = "product: "
VENDOR = "vendor: "
DESCRIPTION = "description: "
SIZE = "size: "
NAME = "logical name: "


def lazy_read(path: Path) -> Iterator[str]:
    with path.open("r") as lines:
        for line in lines:
            yield line.strip()


def get_host(path: Path) -> str:
    with path.open("r") as output:
        return output.readline().strip()


def get_linux(path: Path) -> str:
    linux_version = ""
    with path.open("r") as lines:
        for line in (line.strip() for line in lines):
            if "Description" in line.split(":")[0]:
                linux_version = line.split(":")[1]
                break

    return linux_version


def get_kernel(path: Path) -> str:
    with path.open("r") as output:
        return output.readline().strip()


def read_sysinfo(sysinfo_path: Path) -> Tuple[str, str]:
    """Read output from running the sysinfo cmd"""
    hostname = ""
    linux_version = ""
    kernel_version = ""

    for state_path in sysinfo_path.glob("*.state"):
        with state_path.open("r") as state:
            for line in (line.strip() for line in state):
                output_path = sysinfo_path / state_path.name.replace(
                    ".state", ".output"
                )
                right, left = line.strip().split(":")
                if "cmd" in right:
                    if "hostname" in left:
                        hostname = get_host(output_path)
                    elif "release" in left:
                        linux_version = get_linux(output_path)
                    elif "uname" in left:
                        kernel_version = get_kernel(output_path)

    # Don't include the hostname in the kernel_version
    if hostname and hostname in kernel_version:
        kernel_version = kernel_version.replace(hostname + " ", "")

    return linux_version, kernel_version


def get_motherboard(lines: Iterator[str]) -> str:
    name = ""
    motherboard = ""
    for line in lines:
        if PRODUCT in line:
            name = line.replace(PRODUCT, "")
        elif VENDOR in line:
            motherboard = line.replace(VENDOR, "") + " " + name
            break

    return motherboard


def get_cpu(lines: Iterator[str]) -> str:
    cpu = ""
    for line in lines:
        if PRODUCT in line:
            cpu = line.replace(PRODUCT, "")
            break

    return cpu


def get_ram(lines: Iterator[str]) -> str:
    description = ""
    ram = ""
    for line in lines:
        if DESCRIPTION in line:
            if "[empty]" in line:
                break
            else:
                description = line.replace(DESCRIPTION, "")
        elif SIZE in line:
            ram = line.replace(SIZE, "") + " " + description
            break

    return ram


def get_nvme(lines: Iterator[str]) -> Tuple[str, str]:
    product = ""
    name = ""
    for line in lines:
        if PRODUCT in line:
            product = line.replace(PRODUCT, "")
        elif NAME in line:
            name = line.replace(NAME, "")
            break

    return product, name


def read_hardware(hardware_path: Path) -> Tuple[str, str, List, Dict]:
    """Read output from running the lshw cmd"""
    motherboard = ""
    cpu = ""
    ram = []
    drives = {}
    lshw_output_path = None

    for state_path in hardware_path.glob("*.state"):
        with state_path.open("r") as state:
            for line in state:
                right, left = line.split(":")
                if "cmd" in right:
                    if "lshw" not in left:
                        break
                    else:
                        lshw_output_path = hardware_path / state_path.name.replace(
                            ".state", ".output"
                        )

    if lshw_output_path:
        lines = lazy_read(lshw_output_path)

        for line in lines:
            if "*-core" in line:
                motherboard = get_motherboard(lines)
            elif "*-cpu" in line:
                cpu = get_cpu(lines)
            elif "*-bank" in line:
                bank = get_ram(lines)
                if bank:
                    ram.append(bank)
            elif "*-nvme" in line:
                product, name = get_nvme(lines)
                drives[name] = product

    return motherboard, cpu, ram, drives


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
    prefix_path = Path(step.get("with", {}).get("path", cijoe.output_path)).resolve()
    plot_path = prefix_path / "artifacts"
    sysinfo_path = prefix_path / "001_sysinfo"
    hardware_path = prefix_path / "002_hardware"

    report_path.mkdir(parents=False, exist_ok=False)

    # Fill 'report_path' with files needed to produce the .pdf
    copyfile(templates_path / "style.yaml", style_path, follow_symlinks=False)
    copyfile(templates_path / "xnvme.png", logo_path, follow_symlinks=False)

    # Copy graphs from results/artifacts into 'report_path'
    for png_path in plot_path.glob("*.png"):
        print(png_path)
        copyfile(png_path, report_path / png_path.name)

    linux_version, kernel_version = read_sysinfo(sysinfo_path)

    motherboard, cpu, ram, drives = read_hardware(hardware_path)

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
    template = template_env.get_template("bench.jinja2.rst")
    with body_path.open("w") as body:
        body.write(
            template.render(
                {
                    "linux_version": linux_version,
                    "kernel_version": kernel_version,
                    "motherboard": motherboard,
                    "cpu": cpu,
                    "ram": ram,
                    "drives": drives,
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
