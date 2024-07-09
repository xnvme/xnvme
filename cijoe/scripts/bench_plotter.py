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
from cijoe.core.resources import dict_from_yamlfile

FIO_OUTPUT_NORMALIZED_FILENAME = "fio-output-normalize.json"
BDEVPERF_OUTPUT_NORMALIZED_FILENAME = "bdevperf-output-normalized.json"


def data_as_a_function_of(data, x="iodepth", y="iops", filter=lambda _: True):
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
        err = lineplot(args, cijoe, step)
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
