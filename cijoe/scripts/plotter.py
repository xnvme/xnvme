#!/usr/bin/env python
"""
Helper functions for plotting benchmark graphs
==============================================



.. note::
    This uses matplotlib and numpy for plotting
"""
import logging as log
from pathlib import Path
from typing import Dict, List

import matplotlib.pyplot as plt
from cijoe.core.resources import dict_from_yamlfile

OUTPUT_NORMALIZED_FILENAME = "benchmark-output-normalized.json"
PLOT_SCALE = 1.5
BAR_WIDTH = 0.2

Data = Dict[str, Dict[str, List[float | int] | str]]


def data_as_a_function_of(data, x="iodepth", y="iops", filter=lambda _: True) -> Data:
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


def fmt(i):
    if i > 999:
        return f"{i:,.0f}"
    else:
        return f"{i:,.2f}"


def add_bars_to_plot(data, ax, plot_attributes):
    """
    Given data, a touple containing (x-values, y-values, series-label), add
    the bars to the graph `ax`.
    """
    colors = plot_attributes["styles"].get("colors", ["#000000"])
    hatches = plot_attributes["styles"].get("hatches", ["."])

    for i, (xs, ys, label) in enumerate(data):
        attrs = plot_attributes["legends"].get(label, None)
        if attrs is None:
            log.error(f"Missing plot-attributes for label({label})")
            continue

        bar = ax.bar(
            xs,
            ys,
            BAR_WIDTH,
            label=attrs["legend"],
            color=colors[i % len(colors)],
            hatch=hatches[i % len(hatches)],
        )

        ax.bar_label(bar, fmt=fmt, padding=3, rotation=90)


def draw_scaling_bar_plot(datavalues, plot_attributes, ax):
    """Draw a bar plot with the given data"""

    num_bars = len(datavalues) - 1
    unique_x_values = map(lambda xy: xy[0], list(datavalues)[0]["xys"])
    x_ticks = dict(
        [
            (x, (i + (num_bars * BAR_WIDTH) / 2, x))
            for i, x in enumerate(unique_x_values)
        ]
    )

    multiplier = 0
    bar_data = []

    for samples in datavalues:
        xs, ys = list(zip(*samples["xys"]))

        offset = BAR_WIDTH * multiplier
        plotted_x = [x + offset for x in range(len(xs))]

        bar_data.append((plotted_x, ys, samples["label"]))
        multiplier += 1

    add_bars_to_plot(bar_data, ax, plot_attributes)

    plt.legend(
        bbox_to_anchor=(0, 0.95, 1, 0.2),
        loc="upper center",
        ncol=1,
        facecolor="white",
        framealpha=1,
    )

    ax.set_xticks(
        list(map(lambda val: val[0], x_ticks.values())),
        list(map(lambda val: val[1], x_ticks.values())),
        rotation=0,
        ha="center",
    )


def draw_grouped_bar_plot(datavalues, plot_attributes, ax, fig, groups, y_limit=None):
    """Draw a bar plot where data is separated into the given groups."""

    x_ticks = dict()

    # fiddly stuff to get groups of uneven sizes to have even space between them
    group_sizes = [
        (len(list(filter(lambda item: item["group"] == group, datavalues))) + 1)
        * BAR_WIDTH
        for group in groups
    ]
    x_tick_info = dict(
        [(group, [sum(group_sizes[:i]), 0]) for i, group in enumerate(groups)]
    )

    bar_data = []

    for samples in datavalues:
        label = samples["label"]
        _, ys = list(zip(*samples["xys"]))

        tick_info = x_tick_info[samples["group"]]
        pos = tick_info[0] + tick_info[1] * BAR_WIDTH
        x_tick_info[samples["group"]][1] += 1
        x_ticks[label] = pos, samples["name"]
        plotted_x = [pos]

        bar_data.append((plotted_x, ys, label))

    add_bars_to_plot(bar_data, ax, plot_attributes)

    fig.subplots_adjust(bottom=0.25)

    # add group names at the top of the plot
    group_ticks = list(
        map(
            lambda group: group[0] + (group[1] - 1) / 2 * BAR_WIDTH,
            x_tick_info.values(),
        )
    )
    group_labels = x_tick_info.keys()
    group_axis = ax.secondary_xaxis(location=1)
    group_axis.set_xticks(group_ticks, labels=group_labels)
    group_axis.tick_params("x", length=0)

    # add separators between the groups
    group_separators = [
        sum(group_sizes[:i]) - BAR_WIDTH for i in range(1, len(group_sizes))
    ]
    for tick in group_separators:
        plt.vlines(x=tick, ymin=0, ymax=y_limit, ls=":", lw=1, colors="k")
    group_axis = ax.secondary_xaxis(location=1)
    group_axis.set_xticks(group_separators, labels=[])
    group_axis.tick_params("x", length=15, grid_ls=":")

    ax.set_xticks(
        list(map(lambda val: val[0], x_ticks.values())),
        list(map(lambda val: val[1], x_ticks.values())),
        rotation=40,
        ha="right",
    )


def draw_bar_plot(data, plot_attributes, xlabel=None, ylabel=None, y_limit=None):
    if y_limit is None:
        # map each dataset to its maximum y-value, and find the maximum of these
        # y-values to ensure that all data is within the limits.
        max_y_value = max(
            map(lambda item: max(map(lambda xy: xy[1], item["xys"])), data.values())
        )
        y_limit = max_y_value * PLOT_SCALE

    groups = list(set(map(lambda item: item["group"], data.values())))
    is_grouped = len(groups) > 1

    # Sort to ensure engines will get the same colours across
    # different plots
    datavalues = list(data.values())
    datavalues.sort(key=lambda samples: samples["label"])

    plt.clf()
    fig, ax = plt.subplots()

    if is_grouped:
        draw_grouped_bar_plot(datavalues, plot_attributes, ax, fig, groups, y_limit)
    else:
        draw_scaling_bar_plot(datavalues, plot_attributes, ax)

    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)

    ax.set_ylim([0, y_limit])
