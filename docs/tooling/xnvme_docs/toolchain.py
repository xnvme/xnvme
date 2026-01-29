#!/usr/bin/env python3
"""
xNVMe Toolchain Documentation Generator

Generates the toolchain section of the xNVMe documentation from deps.yaml.

The structured dependency descriptions in ``deps.yaml`` is the primary
data-source for the doc-emitter, the content of ``deps.yaml`` is rendered using
templates/toolchain.{rst,md}.jinja.
"""
from __future__ import annotations

import argparse
import sys
from importlib import resources
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader

DEP_PATH = "deps.yaml"

OS_NAMES: dict[str, str] = {
    "linux": "Linux",
    "freebsd": "FreeBSD",
    "macos": "macOS",
    "windows": "Windows",
}


def get_templates_path() -> Path:
    """Get path to templates directory using importlib.resources."""
    return Path(str(resources.files("xnvme_docs") / "templates"))


def find_project_root(start_path: Path | None = None) -> Path:
    """
    Find the xNVMe project root by searching for meson.build.

    Args:
        start_path: Starting directory for search. Defaults to current directory.

    Returns:
        Path to the project root.

    Raises:
        FileNotFoundError: If project root cannot be found.
    """
    if start_path is None:
        start_path = Path.cwd()

    current = start_path.resolve()
    while current != current.parent:
        if (current / "meson.build").exists() and (current / "toolbox").exists():
            return current
        current = current.parent

    raise FileNotFoundError(
        "Could not find xNVMe project root. "
        "Make sure you're running from within the xNVMe repository."
    )


def find_docs_root(start_path: Path | None = None) -> Path:
    """Find the docs directory."""
    project_root = find_project_root(start_path)
    return project_root / "docs"


def setup() -> argparse.Namespace:
    """Parse command-line arguments."""
    prsr = argparse.ArgumentParser(
        description="xNVMe toolchain documentation generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--format",
        choices=["rst", "md"],
        default="md",
        help="Output format (rst or md)",
    )
    prsr.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output directory (default: docs/toolchain)",
    )

    return prsr.parse_args()


def main() -> int:
    """Entry point for CLI."""
    args = setup()

    try:
        project_root = find_project_root()
        docs_root = find_docs_root()
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    templates_path = get_templates_path()

    # Determine output format and template
    output_format = args.format
    template_name = f"toolchain.{output_format}.jinja"

    template = Environment(
        loader=FileSystemLoader(searchpath=str(templates_path))
    ).get_template(template_name)

    toolbox_path = project_root / "toolbox"
    ypath = toolbox_path / "pkgs" / "emitter" / DEP_PATH
    docs_path = args.output if args.output else docs_root / "toolchain"

    # Load deps.yaml content
    with ypath.resolve().open("r") as dpath:
        content = yaml.safe_load(dpath)

    # Find build scripts
    build_scripts: dict[str, Path] = {}
    for bs in (toolbox_path / "pkgs").glob("*-build*"):
        build_scripts[bs.stem.replace("-build", "")] = bs
    content["build_scripts"] = build_scripts

    # Group platforms by OS
    platforms: dict[str, list[dict]] = {}
    for platform in content["platforms"]:
        osname = platform["os"]
        if osname not in platforms:
            platforms[osname] = []
        platforms[osname].append(platform)

    # Generate documentation for each OS
    ext = output_format
    for oslabel, osname in OS_NAMES.items():
        section_dir = docs_path / oslabel
        section_file = section_dir / f"index.{ext}"

        data = {
            "platforms": platforms.get(oslabel),
            "oslabel": oslabel,
            "osname": osname,
            "build_scripts": build_scripts,
        }

        section_dir.mkdir(parents=True, exist_ok=True)
        with section_file.open("w") as file:
            file.write(template.render(data))

        print(f"Wrote: {section_file}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
