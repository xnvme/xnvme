#!/usr/bin/env python3
"""
Generate the Python or the Rust bindings of the C API with yace

    xnvme_bindings.py {python,rust} <file> [--no-check]

Writes the Python module, or the Rust source of the crate xnvme-sys, to
<file>. yace is run as a module of the Python running this, so run it with
the Python of the environment python/bindings/requirements.txt is installed
in, which pins it.

The bindings cover the headers that libxnvme.h includes, except the headers
that are not part of the API a binding wraps: the command-line helpers, the
deprecated calls kept for compatibility, and the printers of the C API. yace
gives each record printers in the language of the binding. yace checks what
it generates against a C compiler, and for Rust also with cargo, unless
--no-check is given.
"""
import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

INCLUDE = Path(__file__).resolve().parents[1] / "include"
UMBRELLA = "libxnvme.h"
EXCLUDED = ["libxnvme_cli.h", "libxnvme_compat.h", "libxnvme_pp.h"]

# The system headers the API headers use without including them
PRELUDE = [
    "stdio.h",
    "time.h",
    "sys/uio.h",
    "sys/types.h",
    "sys/stat.h",
    "assert.h",
    "inttypes.h",
    "stdlib.h",
    "string.h",
]

# The license of the package each binding is distributed in
LICENSE = {"python": "Apache-2.0", "rust": "BSD-3-Clause"}

# Where yace writes each binding, relative to its output directory
EMITTED = {"python": Path("python") / "xnvme.py", "rust": Path("rust") / "bindings.rs"}


def headers(include=INCLUDE):
    """The headers that libxnvme.h includes, in its order, except EXCLUDED"""

    text = (include / UMBRELLA).read_text()
    found = re.findall(r'^\s*#\s*include\s+"([^"]+)"', text, re.MULTILINE)
    return [name for name in found if name not in EXCLUDED]


def meta(target):
    """yace's meta of the binding for 'target', as YAML"""

    return "\n".join(
        [
            "name: xnvme",
            "prefix: xnvme",
            "brief: The xNVMe C API",
            f"license: {LICENSE[target]}",
            "pkg: xnvme",
            "crate: xnvme-sys",
            "",
        ]
    )


def generate(target, path, check=True, include=INCLUDE):
    """Generate the binding for 'target' into 'path'; returns yace's exit code"""

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "meta.yaml").write_text(meta(target))
        cmd = [
            sys.executable,
            "-m",
            "yace",
            "--meta",
            str(tmp / "meta.yaml"),
            "-I",
            ".",
        ]
        for name in PRELUDE:
            cmd += ["--prelude", name]
        cmd += ["--emit", target, "--output", str(tmp / "out")]
        if not check:
            cmd += ["--no-check"]
        cmd += ["--"] + headers(include)

        proc = subprocess.run(cmd, cwd=include)
        if proc.returncode:
            return proc.returncode

        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(tmp / "out" / EMITTED[target], path)

    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate a binding of the C API with yace"
    )
    parser.add_argument(
        "target", choices=sorted(EMITTED), help="the binding to generate"
    )
    parser.add_argument("path", type=Path, help="the file to write it to")
    parser.add_argument(
        "--no-check",
        dest="check",
        action="store_false",
        help="do not build and check what yace generates",
    )
    args = parser.parse_args(argv)

    return generate(args.target, args.path.resolve(), args.check)


if __name__ == "__main__":
    sys.exit(main())
