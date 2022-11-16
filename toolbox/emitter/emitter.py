#!/usr/bin/env python3
import argparse
import logging
from pathlib import Path

import yaml
from jinja2 import Template


def parse_args():
    """Parse command-line arguments"""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--meta",
        type=Path,
        default=Path("meta.yaml"),
        help="Path to meta definitions such as namespace prefix",
    )
    parser.add_argument(
        "--model",
        type=Path,
        default=Path("model"),
        help="Path to directory containing the data model",
    )
    parser.add_argument(
        "--templates",
        type=Path,
        default=Path("templates"),
        help="Path directory containing code-emitter-templates",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("output"),
        help="Path to store emitted code in",
    )

    return parser.parse_args()


def emit_struct(meta, current, parent):
    """This is a crude code-emission for nested structures / data-layouts"""

    content = []
    ltype = current["ltype"]

    if parent and parent["ltype"] == "layout":
        content.append(f"\n/** {current['brief']} */")

    if ltype == "bitfield":
        for bfield in current["members"]:
            content.append(
                f"{ current['dtype'] } { bfield['symbol'] } : { bfield['val']}; ///< { bfield['brief'] }"
            )
    elif ltype == "pod":
        content.append(
            f"{current['val']} {current['symbol']}; ///< { current['brief'] }"
        )
    elif ltype in ["struct", "union"]:
        content.append(
            f"{ltype} "
            + "%s{"
            % (
                "_".join([meta["prefix"], parent["symbol"], current["symbol"]])
                if parent and parent["ltype"] == "layout"
                else ""
            )
        )
        for member in current["members"]:
            content += emit_struct(meta, member, None)
        content.append(
            "}%s;"
            % (
                current["symbol"]
                if current["symbol"] and not (parent and parent["ltype"] == "layout")
                else ""
            )
        )

    return content


def main(args):
    """Emit spec-structures"""

    logging.basicConfig(level=logging.DEBUG)

    templates = {f.stem: Template(f.open("r").read()) for f in args.templates.iterdir()}

    args.output.mkdir(parents=True, exist_ok=True)

    model = {}
    for path in list(args.model.glob("**/*.yaml")):
        with path.open() as sfd:
            model.update(yaml.safe_load(sfd))

    meta = {}
    with args.meta.open() as sfd:
        meta.update(yaml.safe_load(sfd))

    logging.info("The loaded model has the following 'topics'")
    for topic_name in model.keys():
        logging.info(f"topic({topic_name})")

    content = []
    for topic_name, topic in model.items():
        ltype = topic["ltype"]

        if ltype == "layout":  # Code-emission for "nested" structs
            for layout in topic["members"]:
                content.append("\n".join(emit_struct(meta, layout, topic)))
        else:  # Code-emission for flat enums/values
            content.append(
                templates[ltype].render(
                    meta=meta,
                    model=model,
                    topic_name=topic_name,
                    topic=topic,
                )
            )
    with (args.output / "libxnvme_spec.h").open("w") as hfile:
        hfile.write(
            templates["specheader"].render(content="\n".join(content), meta=meta)
        )


if __name__ == "__main__":
    main(parse_args())
