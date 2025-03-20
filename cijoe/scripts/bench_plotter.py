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
import json
import logging as log
import os
import traceback
from argparse import ArgumentParser
from pathlib import Path

import matplotlib.pyplot as plt
from plotter import (
    OUTPUT_NORMALIZED_FILENAME,
    PLOT_SCALE,
    data_as_a_function_of,
    draw_bar_plot,
    plot_attributes_from_args,
)


def add_args(parser: ArgumentParser):
    parser.add_argument("--path", type=Path, default=None)
    parser.add_argument("--tool", choices=["bdevperf", "fio"], default="fio")
    parser.add_argument("--limits", type=str, default="plot-limits.yaml")
    parser.add_argument("--legends", type=str, default="plot-legends.yaml")
    parser.add_argument("--styles", type=str, default="plot-styles.yaml")


def create_plots(args, cijoe):
    search = args.path or args.output
    if not search:
        return errno.EINVAL
    artifacts = args.output / "artifacts"
    os.makedirs(artifacts, exist_ok=True)

    tool = args.tool
    search_for = OUTPUT_NORMALIZED_FILENAME

    path = next(Path(search).rglob(search_for))
    with path.open() as jfd:
        data = json.load(jfd)

    plot_attributes = plot_attributes_from_args(args)

    x = "iodepth"

    max_y = max(map(lambda item: item[1]["iops"], data))
    all_groups = set(map(lambda item: item[1]["ctx"]["group"], data))

    # Create a plot for each group, showing the scalability of the xNVMe overhead
    # and the memory and cpu usage
    for group in all_groups:
        # Create plot with iodepth on the x-axis and iops on the y-axis
        dset = data_as_a_function_of(
            data, x, "iops", lambda item: item["ctx"]["group"] == group
        )
        draw_bar_plot(
            dset,
            plot_attributes,
            xlabel=x,
            ylabel="iops",
            y_limit=max_y * PLOT_SCALE,
        )
        plt.savefig(artifacts / f"{tool}_iops_barplot_{group}.png")

        # Create plot with iodepth on the x-axis and cpu utilization on the y-axis
        dset = data_as_a_function_of(
            data, x, "cpu", lambda item: item["ctx"]["group"] == group
        )
        draw_bar_plot(
            dset,
            plot_attributes,
            xlabel=x,
            ylabel="percent",
            y_limit=200,
        )
        plt.savefig(artifacts / f"{tool}_cpu_barplot_{group}.png")

    # Create a plot, showing the overhead of xNVMe at iodepth = 1
    dset = data_as_a_function_of(
        data,
        x,
        "lat",
        filter=lambda item: item["ctx"]["group"] != "null"
        and item["ctx"]["iodepth"] == 1
        and item["ctx"]["iosize"] == 4096,
    )
    draw_bar_plot(dset, plot_attributes, ylabel="nanoseconds")

    plt.savefig(artifacts / f"{tool}_barplot_qd1.png")

    return 0


def main(args, cijoe):
    try:
        err = create_plots(args, cijoe)
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
