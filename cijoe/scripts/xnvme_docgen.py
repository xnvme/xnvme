"""
Prepare for documentation generation
====================================

Install requirements/dependencies, check binaries and library paths

Retargetable: True
------------------
"""

import errno
import logging as log
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default="/tmp/xnvme_source",
        help="path to xNVMe source, expected to contain the requirements and dependencies for docgen.",
    )


def main(args, cijoe):
    xnvme_source = args.xnvme_source

    commands = [
        "./toolbox/pkgs/docgen.sh",
        "cd docs/autogen && make deps",
        "xnvme-driver",  # enforce deterministic mapping of pcie to kernel namespaces
        "xnvme-driver reset",  # i.e. 0000:03:000 always becomes nvme0n*
        "xnvme enum",
        "xnvme library-info",
        "pkg-config xnvme --variable=libdir",
        "pkg-config xnvme --variable=datadir",
        "pkg-config xnvme --variable=includedir",
        "sync; echo 1 > /proc/sys/vm/drop_caches",
        "cd docs/autogen && make commands",
        "cd docs/autogen && make docs",
        "cd docs/autogen/builddir/ && tar -czf docs.tar.gz html/.",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    artifacts = Path(cijoe.output_path) / "artifacts"
    cijoe.run_local(f"mkdir -p {artifacts}")
    cijoe.get(
        f"{xnvme_source}/docs/autogen/builddir/docs.tar.gz", f"{artifacts}/docs.tar.gz"
    )

    return first_err
