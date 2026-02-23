"""
Prepare for documentation generation
====================================

Runs kmdo on the remote/guest to capture command outputs, transfers them back,
then builds documentation locally.

Retargetable: True
------------------
"""

import logging as log
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default="/tmp/xnvme_source",
        help="path to xNVMe source on remote/guest",
    )
    parser.add_argument(
        "--local_xnvme_source",
        type=str,
        default="/tmp/artifacts",
        help="path to xNVMe source locally",
    )


def main(args, cijoe):
    # Commands to run on remote/guest (only kmdo - needs NVMe devices)
    remote_commands = [
        "systemctl stop unattended-upgrades 2>/dev/null; systemctl disable unattended-upgrades 2>/dev/null; apt-get -qy remove unattended-upgrades || true",
        "apt-get -qy -o DPkg::Lock::Timeout=120 update",
        "apt-get -qy -o DPkg::Lock::Timeout=120 install bash build-essential git pkg-config python3 python3-venv pipx",
        "pipx ensurepath",
        "pipx install kmdo",
        # Ensure deterministic device naming before capturing command outputs
        "xnvme-driver",
        "xnvme-driver reset",
        "udevadm settle",  # Wait for device nodes to be created
        "ldconfig",
        "sync; echo 1 > /proc/sys/vm/drop_caches",
        # Debug: show available NVMe devices
        "ls -la /dev/nvme* /dev/ng* 2>/dev/null || echo 'No NVMe devices found'",
        "xnvme enum",
        # Capture command outputs with kmdo (run from docs/ for correct relative paths)
        "cd docs && kmdo getting_started --exclude build_",
        "cd docs && kmdo tools",
        "cd docs && kmdo tutorial",
    ]

    first_err = 0

    # Run kmdo commands on remote/guest
    for command in remote_commands:
        err, _ = cijoe.run(command, cwd=args.xnvme_source)
        if err:
            log.error(f"remote cmd({command}), err({err})")
            first_err = first_err if first_err else err

    # Create tarball of full source (now includes kmdo outputs) and transfer back
    log.info("Creating tarball of source on guest...")
    source_tarball = "/tmp/xnvme_source.tar.gz"
    cijoe.run(f"tar -czf {source_tarball} .", cwd=args.xnvme_source)

    log.info("Transferring source tarball from guest...")
    cijoe.run_local(f"mkdir -p {args.local_xnvme_source}")
    cijoe.get(source_tarball, f"{args.local_xnvme_source}/xnvme_source.tar.gz")

    log.info("Unpacking source...")
    cijoe.run_local("tar -xzf xnvme_source.tar.gz", cwd=args.local_xnvme_source)

    # Output kmdo capture files for debugging
    log.info("Dumping kmdo capture files (.cmd, .out, .err)...")
    cijoe.run_local(
        r"find docs -type f \( -name '*.cmd' -o -name '*.out' \) -exec echo '=== {} ===' \; -exec cat {} \;",
        cwd=args.local_xnvme_source,
    )
    log.warning("Dumping error files (.err) - check these for issues:")
    cijoe.run_local(
        r"find docs -type f -name '*.err' -exec echo '*** {} ***' \; -exec cat {} \;",
        cwd=args.local_xnvme_source,
    )

    # Commands to run locally (docs tooling)
    local_commands = [
        "pipx install --force docs/tooling",
        "make docs",
        "cd docs/builddir/ && tar -czf docs.tar.gz html/.",
    ]

    # Run docs building locally
    for command in local_commands:
        err, _ = cijoe.run_local(command, cwd=args.local_xnvme_source)
        if err:
            log.error(f"local cmd({command}), err({err})")
            first_err = first_err if first_err else err

    # Copy final artifact
    artifacts = Path(cijoe.output_path) / "artifacts"
    cijoe.run_local(f"mkdir -p {artifacts}")
    cijoe.run_local(
        f"cp {args.local_xnvme_source}/docs/builddir/docs.tar.gz {artifacts}/docs.tar.gz"
    )

    return first_err
