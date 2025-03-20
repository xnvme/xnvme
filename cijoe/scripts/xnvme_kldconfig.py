"""
Add the SPDK kmod directory to FreeBSD kernel module search path
================================================================

This is needed by the ``xnvme-driver`` script.

Retargetable: True
------------------
"""

from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default="/tmp/xnvme_source",
        help="path to xNVMe source",
    )


def main(args, cijoe):
    osname = cijoe.getconf("os.name", "")
    if osname != "freebsd":
        return 0

    xnvme_source = args.xnvme_source

    cijoe.run(f"kldconfig -i -m -vv {xnvme_source}/subprojects/spdk/dpdk/build/kmod")
    return 0
