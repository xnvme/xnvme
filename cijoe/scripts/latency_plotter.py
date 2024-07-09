#!/usr/bin/env python
"""
Extract and transform fio latency measurements
=============================================

fio has:

* bandwidth in KiB / second
* latency is mean and in nano-seconds
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
import traceback
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

import matplotlib.pyplot as plt
from cijoe.core.resources import dict_from_yamlfile

FIO_OUTPUT_NORMALIZED_FILENAME = "fio-output-normalized.json"

Data = Dict[str, Dict[str, List[float | int] | str]]


def data_as_a_function_of(data, x="iodepth", y="lat", filter=lambda _: True) -> Data:
    """Organize and label data with 'y' points as a function of 'x' points"""

    samples = {}
    for ident, metrics in data:
        context = metrics["ctx"]

        if ident not in samples:
            label = context["name"]
            groups = [
                x.split("=")[1] for x in context["stem"].split("_") if "GROUP" in x
            ]
            samples[ident] = {
                "xs": [],
                "ys": [],
                "ident": ident,
                "label": label,
                "group": groups[0],
            }

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
            groups = [
                x.split("=")[1] for x in context["stem"].split("_") if "GROUP" in x
            ]
            samples[ident] = {
                "xs": [],
                "ys": [],
                "ident": ident,
                "label": label,
                "group": groups[0],
            }

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


def build_plot_groups(data: Data):
    """Group the `data`

    TODO: This functions logic is not sound, it depends on the ordering of the `groups`
    list. The issue presents itself when a partial match on a label occurs.
    """

    grouped = defaultdict(dict)

    for ident, samples in data.items():
        grouped[samples["group"]][ident] = samples
    return grouped


def line_plot(args, cijoe, step):
    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL

    search_for = FIO_OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))

    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = plot_attributes_from_step(step)

    x = "iodepth"
    y = "lat"

    data = data_as_a_function_of(data, x, y)

    grouped = build_plot_groups(data)

    for group, dset in grouped.items():
        plt.clf()

        for _, samples in dset.items():
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
            loc="lower center",
            ncol=2,
            facecolor="white",
            framealpha=1,
        )
        plt.ylim(plot_attributes["limits"]["lineplot"]["y_lim"])

        os.makedirs(args.output / "artifacts", exist_ok=True)
        plt.savefig(args.output / "artifacts" / f"fio_lineplot_{group}.png")

    return 0


def main(args, cijoe, step):
    try:
        err = line_plot(args, cijoe, step)
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
