#!/usr/bin/env python
"""
    Run bdevperf

    Toggling the "conserve_cpu" flag for 'bdev_xnvme'
"""
import logging as log
import os
import shutil
import traceback
from argparse import ArgumentParser
from itertools import product
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--iosizes", type=str, default=[], nargs="+")
    parser.add_argument("--iodepths", type=int, default=[], nargs="+")
    parser.add_argument("--ndevices", type=int, default=1)
    parser.add_argument("--bdev_confs", type=str, default="/tmp")


def main(args, cijoe):
    repetitions = args.repetitions
    iosizes = args.iosizes or ["4K"]
    iodepths = args.iodepths or [1, 2, 4, 8]

    ndevices = args.ndevices
    bdev_confs = args.bdev_confs

    iopaths = {
        "io_uring_cmd-bdev_xnvme": {
            "bdev_name": "bdev_xnvme",
            "io_mechanism": "io_uring_cmd",
        },
        "io_uring_cmd-bdev_xnvme_conserve_cpu": {
            "bdev_name": "bdev_xnvme",
            "io_mechanism": "io_uring_cmd",
        },
        "io_uring-bdev_uring": {
            "bdev_name": "bdev_uring",
            "io_mechanism": "io_uring",
        },
        "io_uring-bdev_xnvme": {
            "bdev_name": "bdev_xnvme",
            "io_mechanism": "io_uring",
        },
        "io_uring-bdev_xnvme_conserve_cpu": {
            "bdev_name": "bdev_xnvme",
            "io_mechanism": "io_uring",
        },
        "libaio-bdev_aio": {
            "bdev_name": "bdev_aio",
            "io_mechanism": "libaio",
        },
        "libaio-bdev_xnvme": {
            "bdev_name": "bdev_xnvme",
            "io_mechanism": "libaio",
        },
        # "libaio-bdev_xnvme_conserve_cpu": {
        #    "bdev_name": "bdev_xnvme",
        #    "io_mechanism": "libaio",
        # },
    }

    artifacts = Path(args.output) / "artifacts"
    os.makedirs(str(artifacts), exist_ok=True)

    err = 0
    try:
        for bs, iodepth, (label, params), rep in product(
            iosizes, iodepths, iopaths.items(), range(repetitions)
        ):
            io_mechanism = params["io_mechanism"]

            linuxperf_output_filename = f"linuxperf-output_BS={bs}_IODEPTH={iodepth}_LABEL={label}_GROUP={io_mechanism}_{rep}.txt"
            bdevperf_output_path = artifacts / (
                f"bdevperf-output_BS={bs}_IODEPTH={iodepth}_LABEL={label}_GROUP={io_mechanism}_{rep}.txt"
            )

            # Create a spdk-configuration file and transfer it
            spdk_conf_path = Path(bdev_confs) / "_".join(
                [
                    params["bdev_name"],
                    io_mechanism,
                    "conserve_cpu" if "conserve_cpu" in label else "",
                    f"{ndevices}.conf",
                ]
            )

            env = {}

            # Run bdevperf
            command = [
                "perf record --call-graph dwarf",
                f"-o {linuxperf_output_filename}",
                "/root/git/spdk/build/examples/bdevperf",
                f"--json {spdk_conf_path}",
                f"-q {iodepth}",
                f"-o {bs}",
                "-w randread",
                "-t 10",
                "-m '[0,1]'",
            ]
            err, state = cijoe.run(" ".join(command), env=env)
            if err:
                log.error(f"failed: {state}")

            # Save the bdevperf output to a file in artifacts directory
            shutil.copyfile(state.output_fpath, bdevperf_output_path)

            command = [
                f"perf report -i {linuxperf_output_filename}",
                "--symbol-filter=bdev",
                "--stdio",
                "--call-graph=none",
            ]
            err, state = cijoe.run(" ".join(command), env=env)
            if err:
                log.error(f"failed: {state}")

            # Save the linuxperf output to a file in artifacts directory
            shutil.copyfile(state.output_fpath, artifacts / linuxperf_output_filename)

            # remove file from server
            err, state = cijoe.run(f"rm {linuxperf_output_filename}", env=env)

    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        print(
            type(exc).__name__,  # TypeError
            __file__,  # /tmp/example.py
            exc.__traceback__.tb_lineno,  # 2
        )

    return err
