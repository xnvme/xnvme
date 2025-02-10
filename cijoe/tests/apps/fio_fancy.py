"""
    fio-wrapper
    ===========

    The intent is to be able to do tie fio into testing infra and to have a means of
    ensuring that "benchmark" can be performed in a reproducible manner.

    It does so by constructing an transforming a dictionary of parameters and
    environment variables. This is done via:

    * fio(cijoe, parameters="", env={}): invoke fio as defined in cijoe.config.options

    * setup_job(param, env, job_name): add parameters JOBS accoring to name

    * setup_ioengine(param, env, ...): add parameters and env. according to ioengine

    * setup_output(param, env, ...): add parameters for output format and file

    * fio_fancy(cijoe, ...): construct parameters using setup_* helpers, then call fio()

    Then **chaining** of the parameter construction is done, such that it is possible to
    control certain job-parameters depending on the device, I/O engine, and I/O engine
    options.
    For example, we might wish to run fio with ``--direct=1``, however, when sending
    that same job to e.g. ``/dev/ng0n1`` (the char-device encapsulating an NVMe
    namespace), then ``--direct=1`` is invalid.
    Such a transformation is then done when seeing that the device is an NVMe chardev.

    config
    ------

    fio.bin

    fio.engines

    retargtable: true
    -----------------
"""

import json
import logging as log
import os
from pathlib import Path

JOBS = {
    "compare": {
        "name": "compare",
        "rw": "randread",
        "size": "12G",
        "iodepth": "1",
        "bs": "4K",
        "direct": "1",
        "thread": "1",
        "time_based": "1",
        "runtime": "7",
        "ramp_time": "3",
        "norandommap": "1",
        "allow_file_create": "0",
    },
    "verify": {
        "name": "verify",
        "rw": "randwrite",
        "size": "1G",
        "iodepth": "16",
        "bs": "4K",
        "direct": "1",
        "thread": "1",
        "verify": "crc32c",
        "allow_file_create": "0",
    },
    "zoned": {
        "name": "zoned",
        "zonemode": "zbd",
        "rw": "write",
        "size": "1G",
        "iodepth": "1",
        "bs": "4K",
        "direct": "1",
        "thread": "1",
        "ramp_time": "1",
        "norandommap": "1",
        "verify": "crc32c",
        "allow_file_create": "0",
    },
}


def setup_job(param, env, job_name):
    """Setup parameters for the fio-job based on the templates in JOBS"""

    param.update(JOBS[job_name])


def setup_ioengine(param, env, engine_name, cijoe, device, xnvme_opts, spdk_opts):
    """
    Setup dictionaries of environment variables and parameters for the given I/O engine

    env: environments variables needed by the ioengine
    param: parameters for the ioengine (param)

    @returns env, param
    """

    engine = (
        cijoe.config.options.get("fio", {}).get("engines", {}).get(engine_name, None)
    )
    if engine is None:
        log.error(f"fio.engine({engine_name}) not in configuration")
        return False

    # disable '--direct' for char-devices
    if "cdev" in device["labels"]:
        param["direct"] = "0"

    # setup the --ioengine flag and possibly the LD_PRELOAD environment variable
    if engine["type"] == "builtin":
        param["ioengine"] = engine_name
    elif engine["type"] == "external_dynamic":
        param["ioengine"] = f"external:{engine['path']}"
    elif engine["type"] == "external_preload":
        param["ioengine"] = engine_name
        env["LD_PRELOAD"] = engine["path"]
    else:
        log.error(f"Configuration has invalid engine.type({engine['type']})")
        return False

    # setup 'xnvme' specific options
    if engine_name == "xnvme":
        if "be" in xnvme_opts:
            param["xnvme_be"] = xnvme_opts["be"]
        if "async" in xnvme_opts:
            param["xnvme_async"] = xnvme_opts["async"]
        if "sync" in xnvme_opts:
            param["xnvme_sync"] = xnvme_opts["sync"]
        if "admin" in xnvme_opts:
            param["xnvme_admin"] = xnvme_opts["admin"]
        if "nsid" in device:
            param["xnvme_dev_nsid"] = device["nsid"]
        if "mem" in xnvme_opts:
            param["xnvme_mem"] = xnvme_opts["mem"]

        if any(label in ["pcie", "fabrics"] for label in device["labels"]):
            param["filename"] = device["uri"].replace(":", r"\:")
        else:
            param["filename"] = device["uri"]
    elif engine_name == "spdk":
        param["filename"] = " ".join(
            [
                "trtype=PCIe",
                f"traddr={device['uri']}",
                f"ns={device['nsid']}",
            ]
        )
    elif engine_name == "spdk_bdev":
        spdk_conf = {
            "subsystems": [
                {
                    "subsystem": "bdev",
                    "config": [
                        {
                            "params": spdk_opts["params"],
                            "method": f"bdev_{spdk_opts['bdev']}_create",
                        }
                    ],
                }
            ]
        }
        conf_filestem = "_".join(
            [
                "spdk",
                "bdev",
                spdk_opts["bdev"],
                spdk_opts["params"].get("io_mechanism", "foo"),
            ]
        )
        spdk_conf_path = Path(f"/tmp/{conf_filestem}.conf")
        with spdk_conf_path.open("w") as scpath:
            json.dump(spdk_conf, scpath)

        cijoe.put(spdk_conf_path, spdk_conf_path)

        param["spdk_conf"] = f"{spdk_conf_path}"
        param["filename"] = f"{spdk_opts['params']['name']}"
    else:
        param["filename"] = device["uri"]


def setup_output(param, env, output_fpath):
    """
    Sets up fio to store output in 'output_fpath' in formated in JSON as well as normal
    output. For postprocessing the json-document needs to be extracted as it is
    intermixed without non-JSON content.
    """

    param["output-format"] = "normal,json"
    param["output"] = f"{output_fpath}"


def fio(cijoe, parameters="", env={}):
    """
    Invoke 'fio' using binary at `'cijoe.config.options.fio.bin`', with the given
    parameters and environment variables (``env``)

    @returns err, state
    """

    fio_bin = cijoe.getconf("fio.bin")
    if not fio_bin:
        log.error("missing config: fio.bin")
        return 1, None

    return cijoe.run(f"{fio_bin} {parameters}", env=env)


def fio_fancy(
    cijoe,
    output_fpath,
    jobname,
    engine_name,
    device,
    xnvme_opts={},
    spdk_opts={},
    aux={},
    env={},
):
    """
    Run fio using helpers for parameter setup for io-engines, store fio output with
    json, collect the output as artifact

    @returns err, state
    """

    param, job_env = {}, {}  # Setup parameters and env.
    setup_job(param, job_env, jobname)
    setup_ioengine(param, job_env, engine_name, cijoe, device, xnvme_opts, spdk_opts)
    setup_output(param, job_env, output_fpath)

    param.update(aux)

    parameters = " ".join([f'--{key}="{val}"' for key, val in param.items()])

    err, _ = cijoe.run(f"rm {output_fpath}")  # Avoid getting old data...
    if err:
        log.info(f"failed removing '{output_fpath}'")

    err, state = fio(cijoe, parameters, env=job_env | env)

    cijoe.run(f"cat {output_fpath}")  # Get the output in runlog

    artifacts = state.output_dpath / "artifacts"  # Collect output as artifact
    os.makedirs(artifacts, exist_ok=True)
    cijoe.get(str(output_fpath), str(artifacts / output_fpath.name))

    return err, state
