#!/usr/bin/env python
"""
Extract and transform fio and bdevperf output
=============================================

fio has:

* bandwidth in KiB / second
* latency is mean and in nano-seconds
* IOPS are 'raw' / base-unit

bdevperf has:

* bandwidth in MiB / second
* IOPS are 'raw' / base-unit

.. note::
    The metric-context need addition of backend-options, the options are
    semi-encoded by ioengine and 'label', however, that is not very precise.

.. note::
    This uses matplotlib and numpy for plotting
"""
import errno
import hashlib
import json
import logging as log
import os
import pprint
import re
import traceback
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from cijoe.core.analyser import to_base_unit

JSON_DUMP = {"indent": 4}

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"

FIO_COMPOUND_FILENAME = "fio-output-compound.json"
FIO_STEM_REGEX = r"fio-output_IOSIZE=(?P<iosize>\d+)_IODEPTH=(?P<iodepth>\d+)_LABEL=(?P<label>.+)_GROUP=(?P<group>.+)"

BDEVPERF_OUTPUT_PREFIX = "bdevperf-output_"
LINUXPERF_OUTPUT_PREFIX = "linuxperf-output_"
OUTPUT_REGEX = r".*BS=(?P<bs>.*)_IODEPTH=(?P<iodepth>\d+)_LABEL=(?P<label>.*)_GROUP=(?P<group>.*)_\d+.txt"
# format of line is:
#         IOPS       MiB/s    Fail/s  TO/s  Average  min    max
# Total : 507668.17  1983.08  0.00    0.00  15.65    10.31  8453.95
TOTAL_PERF_LINE_REGEX = r"Total\s+\:\s+(?P<iops>\d+\.\d+)\s+(?P<bwps>\d+\.\d+)\s+\d+\.\d+\s+\d+\.\d+\s+(?P<lat>\d+\.\d+).*"

# format of the line is:
# Children      Self  Command    Shared Object  Symbol
#   44.20%     0.17%  reactor_1  bdevperf       [.] bdev_xnvme_poll
LINUXPERF_LINE_REGEX = r"\s*(?P<cpu>\d+\.\d+)%.*bdev_.*_poll\n"


def extract_bdevperf(args, cijoe, step):
    """
    Traverse 'args.output' for 'bdevperf-output_*.txt' files, and create:

    * A 'bdevperf-output_*.json' per 'bdevperf-output_*.txt'
    * A single "compound" 'bdevperf-output-compound.json' containing all the discovered
      JSON-documents with the file.stem as key.
    """

    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    artifacts = args.output / "artifacts"

    collection = []

    search = Path(search)
    for bdevperf_path in search.rglob(f"{BDEVPERF_OUTPUT_PREFIX}*.txt"):
        linuxperf_path = bdevperf_path.with_name(
            bdevperf_path.name.replace(BDEVPERF_OUTPUT_PREFIX, LINUXPERF_OUTPUT_PREFIX)
        )

        cpu = 0

        with (linuxperf_path).open() as ofile:
            file_content = ofile.read()
            matches = re.findall(LINUXPERF_LINE_REGEX, file_content)
            if len(matches) != 2:
                log.warning(
                    f"Unexpected amount of CPU usage percentages (expected 2) in {linuxperf_path.name}: {matches}"
                )
                if len(matches) < 2:
                    log.error(
                        "Too few CPU usage percentages, needs at least 2, one for each core."
                    )
                    return errno.EINVAL
                matches = matches[0:2]

            for match in matches:
                cpu += float(match)

        with bdevperf_path.open() as ofile:
            match = re.match(OUTPUT_REGEX, str(bdevperf_path))

            sample = {
                "ctx": {},
                "iops": 0,
                "bwps": 0,
                "lat": 0,
                "stddev": 0,
                "cpu": cpu,  # in %
            }

            for ctx in ["iodepth", "bs"]:
                sample["ctx"][ctx] = int(match.group(ctx))
            for ctx in ["label", "group"]:
                sample["ctx"][ctx] = match.group(ctx)
            sample["ctx"]["name"] = (
                sample["ctx"]["label"].split("-")[1].replace("conserve_cpu", "cc")
            )
            sample["ctx"]["iosize"] = sample["ctx"]["bs"]

            file_content = ofile.read()

            perf_match = re.search(TOTAL_PERF_LINE_REGEX, file_content)
            if perf_match:
                for metric in ["iops", "bwps", "lat"]:
                    sample[metric] = float(perf_match.group(metric))

            m = hashlib.md5()
            m.update(
                str.encode(
                    "".join(
                        [
                            f"{key}={val}"
                            for key, val in sorted(sample["ctx"].items())
                            if key not in ["timestamp", "stem", "iodepth"]
                        ]
                    )
                )
            )

            collection.append((str(m.hexdigest()), sample))

    with (artifacts / OUTPUT_NORMALIZED_FILENAME).open("w") as jfd:
        json.dump(collection, jfd, **JSON_DUMP)

    return 0


def dict_from_fio_output_file(fpath: Path) -> dict:
    """
    The JSON output produced by fio comes mixed with non-JSON output, this function
    attempts to extract the JSON part, parse it and return the JSON-document as a dict.

    On error, an empty dict is returned.
    """

    lines = []

    with fpath.open() as fio_output:
        do_append = False
        for line in fio_output:
            if line.startswith("{"):
                do_append = True

            if do_append:
                lines.append(line)

            if line.startswith("}"):
                break
    try:
        return json.loads("".join(lines))
    except json.decoder.JSONDecodeError:
        return {}


def extract(args, cijoe, step):
    """
    Traverse 'args.output' for 'fio-output_*.txt' files, and create:

    * A 'fio-output_*.json' per 'fio-output_*.txt'
    * A single "compound" 'fio-output-compound.json' containing all the discovered
      JSON-documents with the file.stem as key.
    """

    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    compound = {}

    search = Path(search)
    for path in search.rglob("fio-output_*.txt"):
        jsondict = dict_from_fio_output_file(path)
        with path.with_suffix(".json").open("w") as jfd:
            json.dump(jsondict, jfd, **JSON_DUMP)

        compound[path.stem] = jsondict

    artifacts = args.output / "artifacts"

    with (artifacts / FIO_COMPOUND_FILENAME).open("w") as jfd:
        json.dump(compound, jfd, **JSON_DUMP)
        log.info(f"wrote: {jfd.name}")

    return 0


def normalize(args, cijoe, step):
    """Transform the "compound" fio output into series of 'normalized' data"""

    def contextualize(fio_output: dict, stem) -> dict:
        """Helper function for the transformation of output"""

        assert len(fio_output["jobs"]) == 1

        job = fio_output["jobs"][0]
        options = job["job options"]
        match = re.match(FIO_STEM_REGEX, stem)

        context = {
            "name": options["name"],
            "label": options["name"],
            "timestamp": fio_output["timestamp"],
            "ioengine": options["ioengine"],
            "iosize": int(options["bs"]),
            "rw": options["rw"],
            "size": options["size"],
            "iodepth": int(options["iodepth"]),
            "filename": options["filename"],
            "stem": stem,
            "group": match.group("group"),
        }

        context.update(
            {
                key: options[key]
                for key in [
                    "spdk_conf",
                    "xnvme_be",
                    "xnvme_async",
                    "xnvme_sync",
                    "xnvme_admin",
                    "xnvme_dev",
                ]
                if key in options
            }
        )

        metrics = {
            "ctx": context,
            "iops": to_base_unit(job["read"]["iops_mean"], ""),
            "bwps": to_base_unit(job["read"]["bw_mean"], "KiB"),
            "lat": job["read"]["lat_ns"]["mean"],
            "stddev": to_base_unit(job["read"]["lat_ns"]["stddev"], ""),
            "cpu": 0,  # in %
        }

        m = hashlib.md5()
        m.update(
            str.encode(
                "".join(
                    [
                        f"{key}={val}"
                        for key, val in sorted(context.items())
                        if key not in ["timestamp", "stem", "iodepth", "iosize"]
                    ]
                )
            )
        )

        return str(m.hexdigest()), metrics

    paths = list(Path(args.output).rglob(FIO_COMPOUND_FILENAME))
    if len(paths) != 1:
        log.error(f"Unexpected paths: {paths}")
        return errno.EINVAL

    compound_path = paths[0]
    with compound_path.open() as jfd:
        compound = json.load(jfd)

    collection = [contextualize(results, stem) for stem, results in compound.items()]

    with compound_path.with_name(OUTPUT_NORMALIZED_FILENAME).open("w") as jfd:
        json.dump(collection, jfd, **JSON_DUMP)

    return 0


def main(args, cijoe, step):
    try:
        tool = step.get("with", {}).get("tool", "fio")
        if tool == "fio":
            funcs = [extract, normalize]
        elif tool == "bdevperf":
            funcs = [extract_bdevperf]
        else:
            log.error(f"Cannot normalize with tool {tool}")
            return errno.EINVAL

        for func in funcs:
            err = func(args, cijoe, step)
            if err:
                return err
    except Exception as exc:
        log.error(f"Something failed({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        print(
            type(exc).__name__,  # TypeError
            __file__,  # /tmp/example.py
            exc.__traceback__.tb_lineno,  # 2
        )
        return 1

    return 0
