#!/usr/bin/env python3
"""
xNVMe Documentation Build Tooling

Provides unified CLI commands for building, serving, and cleaning documentation.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def find_docs_root(start_path: Path | None = None) -> Path:
    """
    Find the xNVMe docs root by searching upward for docs/conf.py.

    Args:
        start_path: Starting directory for search. Defaults to current directory.

    Returns:
        Path to the docs directory containing conf.py.

    Raises:
        FileNotFoundError: If docs root cannot be found.
    """
    if start_path is None:
        start_path = Path.cwd()

    current = start_path.resolve()

    # First check if we're in the docs directory itself
    if (current / "conf.py").exists():
        return current

    # Check if we're in docs/tooling
    if current.name == "tooling" and (current.parent / "conf.py").exists():
        return current.parent

    # Search upward for the repository root, then look for docs/conf.py
    while current != current.parent:
        docs_path = current / "docs"
        if (docs_path / "conf.py").exists():
            return docs_path
        current = current.parent

    raise FileNotFoundError(
        "Could not find docs root (directory containing conf.py). "
        "Make sure you're running from within the xNVMe repository."
    )


def find_project_root(docs_root: Path) -> Path:
    """
    Find the xNVMe project root from the docs root.

    Args:
        docs_root: Path to the docs directory.

    Returns:
        Path to the project root (parent of docs/).
    """
    return docs_root.parent


def build_html() -> int:
    """
    Build HTML documentation using Sphinx.

    Returns:
        Exit code (0 for success, non-zero for failure).
    """
    parser = argparse.ArgumentParser(
        description="Build xNVMe HTML documentation",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--docs-root",
        type=Path,
        default=None,
        help="Path to docs directory (auto-detected if not specified)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output directory for built docs (default: docs/builddir/html)",
    )
    parser.add_argument(
        "--fresh",
        action="store_true",
        help="Force fresh build (equivalent to sphinx -E flag)",
    )
    args = parser.parse_args()

    try:
        docs_root = find_docs_root(args.docs_root)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    build_dir = args.output if args.output else docs_root / "builddir" / "html"
    build_dir.mkdir(parents=True, exist_ok=True)

    # Build sphinx command
    cmd = [
        sys.executable,
        "-m",
        "sphinx",
    ]
    if args.fresh:
        cmd.append("-E")
    cmd.extend(
        [
            "-b",
            "html",
            "-c",
            str(docs_root),
            str(docs_root),
            str(build_dir),
        ]
    )

    print(f"Building documentation: {docs_root} -> {build_dir}")
    result = subprocess.run(cmd, cwd=docs_root)
    return result.returncode


def serve() -> int:
    """
    Serve documentation with live reload using sphinx-autobuild.

    Returns:
        Exit code (0 for success, non-zero for failure).
    """
    parser = argparse.ArgumentParser(
        description="Serve xNVMe documentation with live reload",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--docs-root",
        type=Path,
        default=None,
        help="Path to docs directory (auto-detected if not specified)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8000,
        help="Port to serve on",
    )
    parser.add_argument(
        "--host",
        type=str,
        default="127.0.0.1",
        help="Host to bind to",
    )
    parser.add_argument(
        "--open-browser",
        action="store_true",
        help="Open browser automatically",
    )
    args = parser.parse_args()

    try:
        docs_root = find_docs_root(args.docs_root)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    build_dir = docs_root / "builddir" / "html"
    build_dir.mkdir(parents=True, exist_ok=True)

    # Build sphinx-autobuild command
    cmd = [
        sys.executable,
        "-m",
        "sphinx_autobuild",
        "-c",
        str(docs_root),
        "--port",
        str(args.port),
        "--host",
        args.host,
        # Watch the tooling directory for template changes
        "--watch",
        str(docs_root / "tooling" / "xnvme_docs" / "templates"),
    ]

    if args.open_browser:
        cmd.append("--open-browser")

    cmd.extend(
        [
            str(docs_root),
            str(build_dir),
        ]
    )

    print(f"Serving documentation from {docs_root}")
    print(f"Open http://{args.host}:{args.port}/ in your browser")
    result = subprocess.run(cmd, cwd=docs_root)
    return result.returncode


def clean() -> int:
    """
    Clean the documentation build directory.

    Returns:
        Exit code (0 for success, non-zero for failure).
    """
    parser = argparse.ArgumentParser(
        description="Clean xNVMe documentation build artifacts",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--docs-root",
        type=Path,
        default=None,
        help="Path to docs directory (auto-detected if not specified)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        dest="clean_all",
        help="Also clean generated API docs",
    )
    args = parser.parse_args()

    try:
        docs_root = find_docs_root(args.docs_root)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    build_dir = docs_root / "builddir"

    if build_dir.exists():
        print(f"Removing build directory: {build_dir}")
        shutil.rmtree(build_dir)
    else:
        print(f"Build directory does not exist: {build_dir}")

    if args.clean_all:
        # Clean generated API documentation
        api_dirs = ["core", "nvme", "file", "cli", "util"]
        for api_dir in api_dirs:
            api_path = docs_root / "api" / "c" / api_dir
            if api_path.exists():
                for f in api_path.glob("xnvme_*.rst"):
                    print(f"Removing: {f}")
                    f.unlink()
                for f in api_path.glob("xnvme_*.md"):
                    print(f"Removing: {f}")
                    f.unlink()

    print("Clean complete.")
    return 0


if __name__ == "__main__":
    # Default to build_html when run directly
    sys.exit(build_html())
