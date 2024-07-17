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

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"

PLOT_SCALE = 1.5


def data_as_a_function_of(data, x="iodepth", y="iops", filter=lambda _: True):
    """Organize and label data with 'y' points as a function of 'x' points"""

    samples = {}

    for ident, metrics in data:
        context = metrics["ctx"]

        if not filter(metrics):
            continue

        if ident not in samples:
            samples[ident] = {
                "xys": [],
                "ident": ident,
                "label": context["label"],
                "name": context["name"],
                "group": context["group"],
            }

        samples[ident]["xys"].append((context[x], metrics[y]))

    for ident in samples:
        samples[ident]["xys"].sort()

    return samples


def plot_attributes_from_step(step):
    """Load plot-attributes using paths in step"""

    limits_path = Path(step.get("with", {}).get("limits", "plot-limits.yaml"))
    legends_path = Path(step.get("with", {}).get("legends", "plot-legends.yaml"))
    styles_path = Path(step.get("with", {}).get("styles", "plot-styles.yaml"))

    return {
        "limits": dict_from_yamlfile(limits_path),
        "legends": dict_from_yamlfile(legends_path),
        "styles": dict_from_yamlfile(styles_path),
    }


def draw_bar_plot(data, plot_attributes, xlabel=None, ylabel=None, y_limit=None):
    colors = plot_attributes["styles"].get("colors", ["#000000"])
    hatches = plot_attributes["styles"].get("hatches", ["."])
    width = 0.20

    groups = list(set(map(lambda item: item["group"], data.values())))
    is_grouped = len(groups) > 1

    if is_grouped:
        x_ticks = dict()

        # fiddly stuff to get groups of uneven sizes to have even space between them
        group_sizes = [
            (len(list(filter(lambda item: item["group"] == group, data.values()))) + 1)
            * width
            for group in groups
        ]
        x_tick_info = dict(
            [(group, [sum(group_sizes[:i]), 0]) for i, group in enumerate(groups)]
        )
    else:
        num_bars = len(data.items()) - 1
        unique_x_values = map(lambda xy: xy[0], list(data.values())[0]["xys"])
        x_ticks = dict(
            [
                (x, (i + (num_bars * width) / 2, x))
                for i, x in enumerate(unique_x_values)
            ]
        )

    if y_limit is None:
        # map each dataset to its maximum y-value, and find the maximum of these
        # y-values to ensure that all data is within the limits.
        max_y_value = max(
            map(lambda item: max(map(lambda xy: xy[1], item["xys"])), data.values())
        )
        y_limit = max_y_value * PLOT_SCALE

    plt.clf()
    fig, ax = plt.subplots()

    multiplier = 0

    for i, samples in enumerate(data.values()):
        label = samples["label"]
        attrs = plot_attributes["legends"].get(label, None)
        if attrs is None:
            log.error(f"Missing plot-attributes for label({label})")
            continue

        xs, ys = list(zip(*samples["xys"]))

        if is_grouped:
            tick_info = x_tick_info[samples["group"]]
            pos = tick_info[0] + tick_info[1] * width
            x_tick_info[samples["group"]][1] += 1
            x_ticks[label] = pos, samples["name"]
            plotted_x = [pos]
        else:
            offset = width * multiplier
            plotted_x = [x + offset for x in range(len(xs))]

        bar = ax.bar(
            plotted_x,
            ys,
            width,
            label=attrs["legend"],
            color=colors[i % len(colors)],
            hatch=hatches[i % len(hatches)],
        )

        def fmt(i):
            if i > 999:
                return f"{i:,.0f}"
            else:
                return f"{i:,.2f}"

        ax.bar_label(bar, fmt=fmt, padding=3, rotation=90)
        multiplier += 1

    if is_grouped:
        fig.subplots_adjust(bottom=0.25)

        # add group names at the top of the plot
        group_ticks = list(
            map(
                lambda group: group[0] + (group[1] - 1) / 2 * width,
                x_tick_info.values(),
            )
        )
        group_labels = x_tick_info.keys()
        group_axis = ax.secondary_xaxis(location=1)
        group_axis.set_xticks(group_ticks, labels=group_labels)
        group_axis.tick_params("x", length=0)

        # add separators between the groups
        group_separators = [
            sum(group_sizes[:i]) - width for i in range(1, len(group_sizes))
        ]
        for tick in group_separators:
            plt.vlines(x=tick, ymin=0, ymax=y_limit, ls=":", lw=1, colors="k")
        group_axis = ax.secondary_xaxis(location=1)
        group_axis.set_xticks(group_separators, labels=[])
        group_axis.tick_params("x", length=15, grid_ls=":")

    else:
        plt.legend(
            bbox_to_anchor=(0, 0.95, 1, 0.2),
            loc="upper center",
            ncol=1,
            facecolor="white",
            framealpha=1,
        )

    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)

    ax.set_xticks(
        list(map(lambda val: val[0], x_ticks.values())),
        list(map(lambda val: val[1], x_ticks.values())),
        rotation=40 if is_grouped else 0,
        ha="right" if is_grouped else "center",
    )

    ax.set_ylim([0, y_limit])


def create_plots(args, cijoe, step):
    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL
    artifacts = args.output / "artifacts"

    tool = step.get("with", {}).get("tool", "fio")
    search_for = OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))
    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = plot_attributes_from_step(step)

    x = "iodepth"
    y = "iops"

    max_y = max(map(lambda item: item[1][y], data))
    all_groups = set(map(lambda item: item[1]["ctx"]["group"], data))

    # Create a plot for each group, showing the scalability of the xNVMe overhead
    for group in all_groups:
        dset = data_as_a_function_of(
            data, x, y, lambda item: item["ctx"]["group"] == group
        )
        draw_bar_plot(
            dset, plot_attributes, xlabel=x, ylabel="iops", y_limit=max_y * PLOT_SCALE
        )

        os.makedirs(artifacts, exist_ok=True)
        plt.savefig(artifacts / f"{tool}_barplot_{group}.png")

    # Create a plot, showing the overhead of xNVMe at queue depth = 1
    dset = data_as_a_function_of(
        data,
        x=x,
        y="lat",
        filter=lambda item: item["ctx"]["group"] != "null"
        and item["ctx"]["iodepth"] == 1
        and item["ctx"]["iosize"] == 4096,
    )
    draw_bar_plot(dset, plot_attributes, ylabel="nanoseconds")

    os.makedirs(artifacts, exist_ok=True)
    plt.savefig(artifacts / f"{tool}_barplot_qd1.png")

    return 0


def main(args, cijoe, step):
    try:
        err = create_plots(args, cijoe, step)
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
