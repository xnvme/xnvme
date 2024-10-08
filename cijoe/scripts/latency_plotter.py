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

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"

PLOT_SCALE = 1.3

Data = Dict[str, Dict[str, List[float | int] | str]]


def data_as_a_function_of(data, x="iodepth", y="lat", filter=lambda _: True) -> Data:
    """Organize and label data with 'y' points as a function of 'x' points"""

    samples = {}

    for ident, metrics in data:
        context = metrics["ctx"]

        if not filter(metrics):
            continue

        if ident not in samples:
            label = context["name"]
            samples[ident] = {
                "xys": [],
                "ident": ident,
                "label": label,
                "group": context["group"],
            }

        samples[ident]["xys"].append((context[x], metrics[y]))

    for ident in samples:
        samples[ident]["xys"].sort()

    return samples


def get_plot_attributes(step):
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

    num_bars = len(data.items()) - 1
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
        unique_x_values = map(lambda xy: xy[0], list(data.values())[0]["xys"])
        x_ticks = dict(
            [(x, i + (num_bars * width) / 2) for i, x in enumerate(unique_x_values)]
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

    # Sort to ensure engines will get the same colours across
    # different plots
    datavalues = list(data.values())
    datavalues.sort(key=lambda samples: samples["label"])

    for i, samples in enumerate(datavalues):
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
            x_ticks[label] = pos
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
        ax.bar_label(bar, fmt=lambda i: f"{i:,.0f}", padding=3, rotation=90)
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
            loc="lower center",
            ncol=2,
            facecolor="white",
            framealpha=1,
        )

    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)

    ax.set_xticks(
        list(x_ticks.values()),
        list(x_ticks.keys()),
        rotation=50 if is_grouped else 0,
        ha="right" if is_grouped else "center",
    )

    ax.set_ylim([0, y_limit])


def create_plots(args, cijoe, step):
    search = step.get("with", {}).get("path", args.output)
    if not search:
        return errno.EINVAL
    artifacts = args.output / "artifacts"

    search_for = OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))
    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = get_plot_attributes(step)

    y = "lat"

    max_y = max(map(lambda item: item[1][y], data))
    all_groups = set(map(lambda item: item[1]["ctx"]["group"], data))

    # Create a plot for each group, showing the scalability of the xNVMe overhead
    for group in all_groups:
        dset = data_as_a_function_of(
            data,
            x="iodepth",
            y=y,
            filter=lambda item: item["ctx"]["group"] == group
            and item["ctx"]["iosize"] == 4096,
        )
        draw_bar_plot(
            dset,
            plot_attributes,
            xlabel="iodepth",
            ylabel="nanoseconds",
            y_limit=max_y * PLOT_SCALE,
        )

        os.makedirs(artifacts, exist_ok=True)
        plt.savefig(
            artifacts / f"fio_barplot_scalability_GROUP={group}_TYPE=iodepth.png"
        )

    for group in all_groups:
        dset = data_as_a_function_of(
            data,
            x="iosize",
            y=y,
            filter=lambda item: item["ctx"]["group"] == group
            and item["ctx"]["iodepth"] == 1,
        )
        draw_bar_plot(
            dset,
            plot_attributes,
            xlabel="iosize",
            ylabel="nanoseconds",
            y_limit=max_y * PLOT_SCALE,
        )

        os.makedirs(artifacts, exist_ok=True)
        plt.savefig(
            artifacts / f"fio_barplot_scalability_GROUP={group}_TYPE=iosize.png"
        )

    # Create a plot, showing the overhead of xNVMe at queue depth = 1
    dset = data_as_a_function_of(
        data,
        x="iodepth",
        y=y,
        filter=lambda item: item["ctx"]["group"] != "null"
        and item["ctx"]["iodepth"] == 1
        and item["ctx"]["iosize"] == 4096,
    )
    draw_bar_plot(dset, plot_attributes, ylabel="nanoseconds")

    os.makedirs(artifacts, exist_ok=True)
    plt.savefig(artifacts / "fio_barplot_qd1.png")

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
