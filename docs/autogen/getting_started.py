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
    docs_path = Path(__file__).parent.parent
    ypath = toolbox_path / "pkgs" / "emitter" / DEP_PATH
    gspath = docs_path / "getting_started" / "toolchain.rst"

    content = None
    with ypath.resolve().open("r") as dpath:
        content = yaml.safe_load(dpath)

    build_scripts = {}
    for bs in (toolbox_path / "pkgs").glob("*-build*"):
        build_scripts[bs.stem.replace("-build", "")] = bs
    content["build_scripts"] = build_scripts

    with gspath.resolve().open("w") as file:
        file.write(template.render(content))


if __name__ == "__main__":
    main()
