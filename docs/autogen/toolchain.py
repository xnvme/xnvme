#!/usr/bin/env python3
"""
This script emits the toolchain section of the xNVMe documentation

The structured dependency descriptions in ``deps.yaml`` is the primary
data-source for the doc-emitter, the content of ``deps.yaml`` is rendered using:

* templates/toolchain.rst.jinja

  * deps.yaml is opened as 'content'

  * 'content' is augmented with 'build_scripts' (list of *-build.sh) scripts

  * 'content' is passed to template-renderer

  * The file `docs/getting_started/toolchain.rst` is produced by this

"""
import copy
from pathlib import Path
from pprint import pprint

import yaml
from jinja2 import Environment, FileSystemLoader

DEP_PATH = "deps.yaml"


def main():
    template = Environment(
        loader=FileSystemLoader(searchpath=Path(__file__).parent / "templates")
    ).get_template("toolchain.rst.jinja")

    toolbox_path = Path(__file__).parent.parent.parent / "toolbox"
    ypath = toolbox_path / "pkgs" / "emitter" / DEP_PATH
    docs_path = Path(__file__).parent.parent / "toolchain"

    content = None
    with ypath.resolve().open("r") as dpath:
        content = yaml.safe_load(dpath)

    build_scripts = {}
    for bs in (toolbox_path / "pkgs").glob("*-build*"):
        build_scripts[bs.stem.replace("-build", "")] = bs
    content["build_scripts"] = build_scripts

    platforms = {}
    for platform in content["platforms"]:
        osname = platform["os"]
        if osname not in platforms:
            platforms[osname] = []
        platforms[osname].append(platform)

    osnames = {
        "linux": "Linux",
        "freebsd": "FreeBSD",
        "macos": "macOS",
        "windows": "Windows",
    }

    for oslabel, osname in osnames.items():
        section_dir = docs_path / oslabel
        section_file = section_dir / "index.rst"

        data = {
            "platforms": platforms.get(oslabel),
            "oslabel": oslabel,
            "osname": osname,
        }

        section_dir.mkdir(exist_ok=True)
        with section_file.open("w") as file:
            file.write(template.render(data))


if __name__ == "__main__":
    main()
