#!/usr/bin/env python
"""
    Run bdevperf

    Toggling the "conserve_cpu" flag for 'bdev_xnvme'
"""
import json
import logging as log
import os
import shutil
import traceback
from itertools import product
from pathlib import Path


def main(args, cijoe, step):
    repetitions = step.get("with", {}).get("repetitions", 3)
    iosizes = step.get("with", {}).get("iosizes", ["4K"])
    iodepths = step.get("with", {}).get("iodepths", [1, 2, 4, 8])

    ndevices = str(step.get("with", {}).get("ndevices", "1"))
    bdev_confs = step.get("with", {}).get("bdev_confs", "/tmp")

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
            bdevperf_output_path = (
                artifacts
                / f"bdevperf-output_BS={bs}_IODEPTH={iodepth}_LABEL={label}_{rep}.txt"
            )

            # Create a spdk-configuration file and transfer it
            spdk_conf_path = Path(bdev_confs) / "_".join(
                [
                    params["bdev_name"],
                    params["io_mechanism"],
                    "conserve_cpu" if "conserve_cpu" in label else "",
                    f"{ndevices}.conf",
                ]
            )

            env = {}

            # Run bdevperf
            command = [
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

    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        print(
            type(exc).__name__,  # TypeError
            __file__,  # /tmp/example.py
            exc.__traceback__.tb_lineno,  # 2
        )

    return err
