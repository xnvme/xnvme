#!/usr/bin/env python3
"""
xNVMe API Documentation Generator

Parses xNVMe header files and generates documentation from ctags output.
"""
from __future__ import annotations

import argparse
import copy
import logging
import sys
from importlib import resources
from pathlib import Path

import jinja2

DECLARATIONS: dict[str, list[str]] = {
    "func": [],
    "struct": [],
    "enum": [],
}

NAMESPACES: dict[str, set[str]] = {
    "core": {
        "xnvme_buf",
        "xnvme_cmd",
        "xnvme_dev",
        "xnvme_geo",
        "xnvme_ident",
        "xnvme_mem",
        "xnvme_opts",
        "xnvme_queue",
        "xnvme_scan",
    },
    "nvme": {
        "xnvme_adm",
        "xnvme_kvs",
        "xnvme_nvm",
        "xnvme_pi",
        "xnvme_spec",
        "xnvme_spec_fs",
        "xnvme_spec_pp",
        "xnvme_topology",
        "xnvme_znd",
    },
    "file": {
        "xnvme_file",
    },
    "cli": {
        "xnvme_cli",
    },
    "util": {
        "xnvme_be",
        "xnvme_lba",
        "xnvme_libconf",
        "xnvme_pp",
        "xnvme_util",
        "xnvme_ver",
    },
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
        if (current / "meson.build").exists() and (current / "include").exists():
            return current
        current = current.parent

    raise FileNotFoundError(
        "Could not find xNVMe project root. "
        "Make sure you're running from within the xNVMe repository."
    )


def symbols(tags_path: Path) -> dict[str, dict[str, list[str]]]:
    """
    Parse ctags file and return symbols grouped by namespaces.

    Args:
        tags_path: Path to the ctags file.

    Returns:
        Dictionary mapping namespace to declarations (func, struct, enum).
    """
    syms: dict[str, dict[str, list[str]]] = {}

    with tags_path.open() as cfd:
        for line in cfd:
            if not line.startswith("xnvme_"):
                # Skip macros etc.
                continue

            parts = line.split("\t")
            symb = parts[0].strip()
            file = parts[1].strip()
            symtype = parts[3].strip()

            namespace = file.removeprefix("include/lib").removesuffix(".h")

            if namespace not in syms:
                syms[namespace] = copy.deepcopy(DECLARATIONS)

            if symtype in ["p", "f"]:
                syms[namespace]["func"].append(symb)
            elif symtype == "s":
                syms[namespace]["struct"].append(symb)
            elif symtype == "g":
                syms[namespace]["enum"].append(symb)
            elif symtype == "d":
                # We don't document our macros
                pass
            else:
                logging.error("Unhandled symtype: %r", symtype)

    return syms


def find_pp(api: dict[str, list[str]]) -> dict[str, list[str]]:
    """
    Find pretty-printers in the API.

    Args:
        api: API declarations dictionary.

    Returns:
        Dictionary mapping data types to their pretty-printer functions.
    """
    pps: dict[str, list[str]] = {}

    for symb in api["func"]:
        parts = symb.split("_")
        dtype = "_".join(parts[:-1])
        tail = parts[-1]

        if tail in ["str", "fpr", "pr"]:
            if dtype not in pps:
                pps[dtype] = []
            pps[dtype].append(symb)

    return pps


def emit(
    namespace: str,
    api: dict[str, list[str]],
    templates_dir: Path,
    output_format: str = "md",
) -> str:
    """
    Render documentation for a namespace.

    Args:
        namespace: The namespace name.
        api: API declarations dictionary.
        templates_dir: Path to templates directory.
        output_format: Output format ("rst" or "md").

    Returns:
        Rendered template string.
    """
    template_loader = jinja2.FileSystemLoader(searchpath=str(templates_dir))
    template_env = jinja2.Environment(loader=template_loader)

    template_name = f"api_section.{output_format}.jinja"
    template = template_env.get_template(template_name)

    project_root = find_project_root()

    return template.render(
        {
            "ns": namespace,
            "api": api,
            "show_header": (project_root / "include" / f"lib{namespace}.h").exists(),
            "show_enums": api["enum"],
            "show_structs": api["struct"],
            "show_funcs": api["func"],
        }
    )


def namespace_to_section(namespace: str) -> str | None:
    """
    Map a namespace to its documentation section.

    Args:
        namespace: The namespace name.

    Returns:
        Section name or None if not found.
    """
    for section, namespaces in NAMESPACES.items():
        if namespace in namespaces:
            return section
    return None


def setup() -> argparse.Namespace:
    """Parse command-line arguments and setup logger."""
    prsr = argparse.ArgumentParser(
        description="xNVMe API documentation generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--tags",
        type=Path,
        help="Path to ctags file",
        required=True,
    )
    prsr.add_argument(
        "--output",
        type=Path,
        default=Path("."),
        help="Path to directory in which to emit documentation",
    )
    prsr.add_argument(
        "--format",
        choices=["rst", "md"],
        default="md",
        help="Output format (rst or md)",
    )
    prsr.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Logging level",
    )

    args = prsr.parse_args()
    args.tags = args.tags.resolve()
    args.output = args.output.resolve()

    logging.basicConfig(
        format="%(asctime)s %(message)s",
        level=getattr(logging, args.log_level.upper(), None),
    )

    return args


def main() -> int:
    """Entry point for CLI."""
    args = setup()

    logging.info("Output: %r", args.output)
    logging.info("Format: %r", args.format)

    syms = symbols(args.tags)

    # Determine templates directory
    templates_dir = get_templates_path()

    for namespace, val in syms.items():
        logging.info("Processing namespace: %r", namespace)

        section = namespace_to_section(namespace)
        if section is None:
            logging.error(
                "No section for namespace(%s); add it to apigen.NAMESPACES",
                namespace,
            )
            return 1

        # Output file with appropriate extension
        ext = args.format
        out_fpath = args.output / "c" / section / f"{namespace}.{ext}"
        out_fpath.parent.mkdir(parents=True, exist_ok=True)

        content = emit(namespace, val, templates_dir, args.format)

        with out_fpath.open("w") as wfd:
            wfd.write(content)

        logging.info("Wrote: %s", out_fpath)

    return 0


if __name__ == "__main__":
    sys.exit(main())
