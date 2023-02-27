#!/usr/bin/env python3
"""
This script produces dependency installation scripts for the software and
platforms in "deps.yaml"

Refactor:

* "src" to "git"
* "sys" to "pkg"
"""
import copy
from pathlib import Path
from pprint import pprint

import yaml
from jinja2 import Environment, FileSystemLoader

DEP_PATH = "deps.yaml"


def load_templates():
    """Returns a dictionary of templates"""

    env = Environment(
        loader=FileSystemLoader(searchpath=Path(__file__).parent / "templates")
    )

    templates = {"script": {}, "pkgman": {}, "src": {}}  # Setup templates
    for tpl_filename in [tpl for tpl in env.list_templates(extensions=["jinja"])]:
        tpl = env.get_template(tpl_filename)
        stem, *_ = tpl_filename.split(".sh.jinja")
        comp, *tail = stem.split("-")

        pkgman, *specialized = tail
        if pkgman not in templates[comp]:
            templates[comp][pkgman] = {}

        if len(tail) == 1:
            templates[comp][pkgman]["default"] = tpl
        elif len(tail) in [2, 3, 4]:
            templates[comp][pkgman]["-".join(specialized)] = tpl

    return templates


def load_deps_transformed():
    """Loads the dependencies from YAML file"""

    content = None
    with (Path(__file__).parent / DEP_PATH).resolve().open("r") as dpath:
        content = yaml.safe_load(dpath)

    origins = {a: dep for c in ["tools", "libs"] for a, dep in content[c].items()}

    for platform, ver in ((p, v) for p in content["platforms"] for v in p["ver"]):
        apps = content["apps"]  # TODO: filter this based on apps on platform..

        # Populate with defaults
        plain = {}
        for category in ["tools", "libs"]:
            for origin, aliases in content["defaults"][platform["os"]][
                category
            ].items():
                if origin not in plain:
                    plain[origin] = []
                for alias in aliases:
                    if alias in platform["drop"]:
                        continue
                    plain[origin].append(alias)
                plain[origin] = list(set(plain[origin]))

        # Adjust to "alternative" origins
        for origin, aliases in platform["alt"].items():
            plain["sys"] = list(set(plain["sys"]) - set(aliases))
            plain[origin] = aliases

        # Resolve the plain into deps by retrieving the names as per "origin"
        deps = {"sys": copy.deepcopy(platform["aux"])}
        for origin, aliases in plain.items():
            if origin not in deps:
                deps[origin] = []

            for alias in aliases:  # Retrieve the package-name from alias
                pkg_names = origins.get(alias, {}).get("names", {})
                if origin == "sys":
                    key = platform["mng"] if platform["mng"] in pkg_names else "default"
                    deps[origin] += pkg_names.get(key, [])
                elif origin == "src":
                    deps[origin].append(alias)
                    continue
                else:
                    deps[origin] += pkg_names.get(origin, [])
                    continue

            deps[origin].sort()

        yield platform, ver, apps, deps


def emit_scripts(templates):
    """Fooo"""

    for platform, ver, apps, deps in load_deps_transformed():
        shell = "batch" if platform["os"] in ["windows"] else "shell"
        suffix = ".bat" if shell == "batch" else ".sh"
        ident = f"{platform['name']}-{ver}"
        script_filename = Path(__file__).parent.with_name(ident + suffix)

        lines = templates["script"][shell]["default"].render()
        lines += "\n\n"

        if "sys" in deps and deps["sys"]:
            lines += (
                templates["pkgman"][platform["mng"]]
                .get(ident, templates["pkgman"][platform["mng"]]["default"])
                .render(apps=apps, deps=deps["sys"], platform=platform)
            )
            lines += "\n\n"

        if "src" in deps:
            for name in deps["src"]:
                lines += (
                    templates["src"][name]
                    .get(ident, templates["src"][name]["default"])
                    .render(apps=apps, deps=deps["sys"], platform=platform)
                )
                lines += "\n\n"

        for pkgman, packages in deps.items():
            if pkgman in ["sys", "src"]:
                continue
            if not packages:
                continue

            lines += (
                templates["pkgman"][pkgman]
                .get(ident, templates["pkgman"][pkgman]["default"])
                .render(apps=apps, deps=deps[pkgman], platform=platform)
            )
            lines += "\n\n"

        with script_filename.resolve().open("w") as script:
            script.write(lines)


def main():
    """Parse arguments here"""

    templates = load_templates()

    emit_scripts(templates)


if __name__ == "__main__":
    main()
