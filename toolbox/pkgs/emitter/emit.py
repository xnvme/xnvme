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

    origins = {a: dep for a, dep in content["deps"].items()}

    for platform, ver in ((p, v) for p in content["platforms"] for v in p["ver"]):
        apps = content["apps"]  # TODO: filter this based on apps on platform..

        # Populate with defaults
        plain = copy.deepcopy(content["defaults"][platform["os"]]["deps"])

        # Remove some deps depending on plaform
        for origin, aliases in plain:
            for alias in platform["drop"]:
                if alias in aliases:
                    aliases.remove(alias)

        # Add platform specific auxiliary tools and libs
        plain += platform["aux"]

        # Adjust to "alternative" origins
        for org_origin, org_aliases in plain:
            for alt_origin, alt_aliases in platform["alt"]:
                for alt_alias in alt_aliases:
                    if alt_alias in org_aliases:
                        org_aliases.remove(alt_alias)

        plain += platform["alt"]

        # Find the first entry of "sys"
        first_sys_entry = -1
        for i, [origin, aliases] in enumerate(plain):
            if origin == "sys":
                first_sys_entry = i
                break

        # Merge all system packages into first entry of "sys"
        for i in reversed(range(len(plain))):
            if plain[i][0] == "sys" and i != first_sys_entry:
                plain[first_sys_entry][1] += plain[i][1]
                del plain[i]

        # Resolve the plain into deps by retrieving the names as per "origin"
        deps = []
        for origin, aliases in plain:
            deps_aliases = []
            for alias in aliases:  # Retrieve the package-name from alias
                pkg_names = origins.get(alias, {}).get("names", {})
                if origin == "sys":
                    key = platform["mng"] if platform["mng"] in pkg_names else "default"
                    deps_aliases += pkg_names.get(key, [alias])
                elif origin == "src":
                    deps_aliases.append(alias)
                    continue
                else:
                    deps_aliases += pkg_names.get(origin, [alias])
                    continue
            deps_aliases.sort()
            deps.append([origin, deps_aliases])

        yield platform, ver, apps, deps


def emit_scripts(templates):
    """Emit shell scripts for each platform"""

    for platform, ver, apps, deps in load_deps_transformed():
        shell = "bash"
        suffix = ".sh"
        if platform["os"] in ["windows"]:
            shell = "batch"
            suffix = ".bat"
        elif platform["os"] in ["freebsd"]:
            shell = "tcsh"

        ident = f"{platform['name']}-{ver}"
        script_filename = Path(__file__).parent.with_name(ident + suffix)

        lines = templates["script"][shell]["default"].render()
        lines += "\n\n"

        for origin, aliases in deps:
            if not aliases:
                continue

            if origin == "sys":
                lines += (
                    templates["pkgman"][platform["mng"]]
                    .get(ident, templates["pkgman"][platform["mng"]]["default"])
                    .render(apps=apps, deps=aliases, platform=platform)
                )
                lines += "\n\n"
            elif origin == "src":
                for alias in aliases:
                    lines += (
                        templates["src"][alias]
                        .get(ident, templates["src"][alias]["default"])
                        .render(apps=apps, deps=aliases, platform=platform)
                    )
                    lines += "\n\n"
            else:
                lines += (
                    templates["pkgman"][origin]
                    .get(ident, templates["pkgman"][origin]["default"])
                    .render(apps=apps, deps=aliases, platform=platform)
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
