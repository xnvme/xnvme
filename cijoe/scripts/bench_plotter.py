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
from cijoe.core.resources import dict_from_yamlfile
from cijoe.fio.wrapper import dict_from_fio_output_file

JSON_DUMP = {"indent": 4}

FIO_COMPOUND_FILENAME = "fio-output-compound.json"
FIO_OUTPUT_NORMALIZED_FILENAME = "fio-output-normalize.json"

BDEVPERF_OUTPUT_PREFIX = "bdevperf-output_"
BDEVPERF_OUTPUT_NORMALIZED_FILENAME = "bdevperf-output-normalized.json"
OUTPUT_REGEX = r".*BS=(?P<bs>.*)_IODEPTH=(?P<iodepth>\d+)_LABEL=(?P<label>.*)_\d+.txt"
LINE_REGEX = r".*Total\s+\:\s+(?P<iops>\d+\.\d+)\s+\s+(?P<bwps>\d+\.\d+)\s+\d+\.\d+\s+\d+\.\d+\s+(?P<lat>\d+\.\d+).*"


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

    collection = []

    search = Path(search)
    for path in search.rglob(f"{BDEVPERF_OUTPUT_PREFIX}*.txt"):
        with path.open() as ofile:
            match = re.match(OUTPUT_REGEX, str(path))

            sample = {
                "ctx": {},
                "iops": 0,
                "bwps": 0,
                "lat": 0,
                "stddev": 0,
            }

            for ctx in ["bs", "iodepth", "label"]:
                if ctx == "iodepth":
                    sample["ctx"][ctx] = int(match.group(ctx))
                else:
                    sample["ctx"][ctx] = match.group(ctx)
            sample["ctx"]["name"] = sample["ctx"]["label"]

            for line in ofile.readlines():
                metrics = re.match(LINE_REGEX, line)
                if metrics:
                    for metric in ["iops", "bwps"]:
                        sample[metric] = float(metrics.group(metric))

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

    artifacts = search / "artifacts"

    with (artifacts / BDEVPERF_OUTPUT_NORMALIZED_FILENAME).open("w") as jfd:
        json.dump(collection, jfd, **JSON_DUMP)

    return 0


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

    artifacts = search / "artifacts"

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

        context = {
            "name": options["name"],
            "timestamp": fio_output["timestamp"],
            "ioengine": options["ioengine"],
            "bs": options["bs"],
            "rw": options["rw"],
            "size": options["size"],
            "iodepth": int(options["iodepth"]),
            "filename": options["filename"],
            "stem": stem,
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
        }

        m = hashlib.md5()
        m.update(
            str.encode(
                "".join(
                    [
                        f"{key}={val}"
                        for key, val in sorted(context.items())
                        if key not in ["timestamp", "stem", "iodepth"]
                    ]
                )
            )
        )

        return str(m.hexdigest()), metrics

    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    paths = list(Path(search).rglob(FIO_COMPOUND_FILENAME))
    if len(paths) != 1:
        log.error(f"Unexpected paths: {paths}")
        return errno.EINVAL

    compound_path = paths[0]
    with compound_path.open() as jfd:
        compound = json.load(jfd)

    collection = [contextualize(results, stem) for stem, results in compound.items()]

    with compound_path.with_name(FIO_OUTPUT_NORMALIZED_FILENAME).open("w") as jfd:
        json.dump(collection, jfd, **JSON_DUMP)

    return 0


def data_as_a_function_of(data, x="iodepth", y="iops"):
    """Organize and label data with 'y' points as a function of 'x' points"""

    samples = {}
    for ident, metrics in data:
        context = metrics["ctx"]

        if ident not in samples:
            label = context["name"]
            samples[ident] = {"xs": [], "ys": [], "ident": ident, "label": label}

        samples[ident]["xs"].append(context[x])
        samples[ident]["ys"].append(metrics[y])

    return samples


def data_as_fixed(data, x="iodepth", y="iops", xval=None):
    """Organize and label data with 'y' points as a function of 'x' points"""

    samples = {}
    for ident, metrics in data:
        context = metrics["ctx"]
        if str(context[x]) != str(xval):
            continue

        if ident not in samples:
            label = context["name"]
            samples[ident] = {"xs": [], "ys": [], "ident": ident, "label": label}

        samples[ident]["xs"].append(context[x])
        samples[ident]["ys"].append(metrics[y])

    return samples


def plot_attributes_from_step(step):
    """Load plot-attributes using paths in step"""

    limits_path = Path(step.get("with", {}).get("limits", "plot-limits.yaml"))
    legends_path = Path(step.get("with", {}).get("legends", "plot-legends.yaml"))

    return {
        "limits": dict_from_yamlfile(limits_path),
        "legends": dict_from_yamlfile(legends_path),
    }


def lineplot(args, cijoe, step):
    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    tool = step.get("with", {}).get("tool", "fio")
    if tool == "fio":
        search_for = FIO_OUTPUT_NORMALIZED_FILENAME
    elif tool == "bdevperf":
        search_for = BDEVPERF_OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))
    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = plot_attributes_from_step(step)

    groups = ["io_uring_cmd", "io_uring", "libaio", "ioctl"]
    x = "iodepth"
    y = "iops"

    data = data_as_a_function_of(data, x, y)
    grouped = {}

    for ident, samples in data.items():
        for group in groups:
            if f"{group}" not in samples["label"]:
                continue
            if group not in grouped.keys():
                grouped[group] = {}

            grouped[group][ident] = samples
            break

    for group, dset in grouped.items():
        plt.clf()

        for ident, samples in dset.items():
            label = samples["label"]
            attrs = plot_attributes["legends"].get(label, None)
            if attrs is None:
                log.error(f"Missing plot-attributes for label({label})")
                continue

            xs, ys = [list(t) for t in zip(*sorted(zip(samples["xs"], samples["ys"])))]
            plt.plot(
                xs,
                ys,
                label=attrs["legend"],
                color=attrs["color"],
                marker=attrs["marker"],
            )

        plt.xlabel(x)
        plt.ylabel(y)
        plt.legend(
            bbox_to_anchor=(0, 0.95, 1, 0.2),
            loc="lower left",
            ncol=2,
            facecolor="white",
            framealpha=1,
        )
        plt.ylim(plot_attributes["limits"]["lineplot"]["y_lim"])

        os.makedirs(args.output / "artifacts", exist_ok=True)
        plt.savefig(args.output / "artifacts" / f"{tool}_lineplot_{group}.png")

    return 0


def barplot(args, cijoe, step):
    """Do a bar plot"""

    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    tool = step.get("with", {}).get("tool", "fio")
    if tool == "fio":
        search_for = FIO_OUTPUT_NORMALIZED_FILENAME
    elif tool == "bdevperf":
        search_for = BDEVPERF_OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))
    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = plot_attributes_from_step(step)

    groups = ["io_uring_cmd", "io_uring", "libaio", "ioctl"]
    y = "iops"
    x = "iodepth"
    xval = "1"

    data = data_as_fixed(data, x, y, xval)
    grouped = {}
    print(tool, grouped, data)

    for ident, samples in data.items():
        for group in groups:
            if f"{group}" not in samples["label"]:
                continue
            if group not in grouped.keys():
                grouped[group] = {}

            grouped[group][ident] = samples
            break

    for group, dset in grouped.items():
        plt.clf()

        nsamples = len(dset.keys())
        index = np.arange(nsamples)

        bars = []
        plots = []
        labels = []
        for index, (ident, samples) in enumerate(dset.items()):
            label = samples["label"]
            attrs = plot_attributes["legends"].get(label, None)
            if attrs is None:
                log.error(f"Missing plot-attributes for label({label})")
                continue

            ys = samples["ys"]
            dist = max(ys) - min(ys)
            mean = sum(ys) / len(ys)
            barplot = plt.bar(
                index,
                mean,
                width=0.9 / nsamples,
                yerr=dist,
                color=[attrs["color"]],
                hatch=attrs["hatch"],
            )
            plt.text(index, mean + dist, f"{mean:.2f}", ha="center")
            bars.append(barplot)
            plots.append(barplot[0])
            labels.append(attrs["legend"])

        plt.xlabel(f"{x}={xval}")
        plt.ylabel(y)
        plt.legend(
            plots,
            labels,
            bbox_to_anchor=(0, 0.95, 1, 0.2),
            loc="lower left",
            ncol=2,
            facecolor="white",
            framealpha=1,
        )
        plt.ylim(plot_attributes["limits"]["barplot"]["y_lim"])

        os.makedirs(args.output / "artifacts", exist_ok=True)
        plt.savefig(args.output / "artifacts" / f"{tool}_barplot_{group}.png")


def main(args, cijoe, step):
    try:
        search = step.get("with", {}).get("path", args.output)
        tool = step.get("with", {}).get("tool", "fio")

        if not search:
            return errno.EINVAL

        if tool == "fio":
            funcs = [extract, normalize, lineplot, barplot]
        elif tool == "bdevperf":
            funcs = [extract_bdevperf, lineplot, barplot]

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
