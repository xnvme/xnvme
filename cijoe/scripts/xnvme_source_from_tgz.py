"""
Prepare xNVMe source directory using GitHUB Artifacts
=====================================================

xNVMe GitHUB artifacts:

* xnvme-src.tar.gz
* xnvme-py-sdist.tar.gz

Transfers the above artifacts from step.with.artifacts, to
step.with.xnvme_sources and extracts the content of xnvme.tar.gz

Retargetable: True
------------------
"""

import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--artifacts",
        type=str,
        default="/tmp/artifacts",
        help="path to where the GitHub artifacts are located locally",
    )
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default="/tmp/xnvme_source",
        help="path to where the GitHUB artifacts should transferred to",
    )


def main(args, cijoe):
    """Transfer artifacts"""

    artifacts = args.artifacts
    xnvme_source = args.xnvme_source

    _, _ = cijoe.run(
        f'[ -d "{xnvme_source}" ] && mv "{xnvme_source}" "{xnvme_source}-$(date +%Y%m%d%H%M%S)" || true'
    )

    err, _ = cijoe.run(f"mkdir -p {xnvme_source}")
    if err:
        return err

    for filename in ["xnvme-src.tar.gz", "xnvme-py-sdist.tar.gz"]:
        cijoe.put(
            f"{artifacts}/{filename}",
            f"{xnvme_source}/{filename}",
        )

    commands = [
        f"ls -lh {xnvme_source}",
        "tar xzf xnvme-src.tar.gz --strip 1",
        "rm xnvme-src.tar.gz",
        "df -h",
        "ls -lh",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=f"{xnvme_source}")
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
