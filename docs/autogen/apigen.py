#!/usr/bin/env python3
"""
    Parse the xNVMe header files for auto-generation purposes
"""
from __future__ import print_function

import argparse
import copy
import logging
import os
import sys
from subprocess import PIPE, Popen

import jinja2

DECLARATIONS = {
    "func": [],
    "struct": [],
    "enum": [],
}  # type: dict


NAMESPACES = {
    "core": set(
        [
            "xnvme_buf",
            "xnvme_cmd",
            "xnvme_dev",
            "xnvme_geo",
            "xnvme_ident",
            "xnvme_mem",
            "xnvme_opts",
            "xnvme_queue",
        ]
    ),
    "nvme": set(
        [
            "xnvme_adm",
            "xnvme_kvs",
            "xnvme_nvm",
            "xnvme_pi",
            "xnvme_spec",
            "xnvme_spec_fs",
            "xnvme_spec_pp",
            "xnvme_topology",
            "xnvme_znd",
        ]
    ),
    "file": set(
        [
            "xnvme_file",
        ]
    ),
    "cli": set(
        [
            "xnvme_cli",
        ]
    ),
    "util": set(
        [
            "xnvme_be",
            "xnvme_lba",
            "xnvme_libconf",
            "xnvme_pp",
            "xnvme_util",
            "xnvme_ver",
        ]
    ),
}


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def run(cmd, cmd_input=None):
    """Run the given 'cmd' and return (stdout, stderr, returncode)"""

    with Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
        out, err = proc.communicate(
            input=cmd_input.encode("utf-8") if cmd_input else None
        )
        rcode = proc.returncode

        return out.decode("utf-8"), err.decode("utf-8"), rcode


def setup():
    """Parse command-line arguments for generator and setup logger"""

    prsr = argparse.ArgumentParser(
        description="xNVMe header parser",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--tags",
        help="Path to ctags file",
        required=True,
    )
    prsr.add_argument(
        "--output",
        help="Path to directory in which to emit parse-artifacts",
        default=os.sep.join(["."]),
    )
    prsr.add_argument(
        "--log-level",
        help="log-devel",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
    )

    args = prsr.parse_args()
    args.tags = expand_path(args.tags)
    args.output = expand_path(args.output)

    logging.basicConfig(
        format="%(asctime)s %(message)s",
        level=getattr(logging, args.log_level.upper(), None),
    )

    return args


def symbols(args):
    """Returns symbols grouped by namespaces"""

    syms = {}

    with open(args.tags) as cfd:
        for line in cfd.readlines():
            if not line.startswith("xnvme_"):
                # Skip macros etc.
                continue

            symb = line.split("\t")[0].strip()
            file = line.split("\t")[1].strip()
            symtype = line.split("\t")[3].strip()

            namespace = file.removeprefix("include/lib").removesuffix(".h")

            if namespace not in syms:
                syms[namespace] = copy.deepcopy(DECLARATIONS)

            if symtype in ["p", "f"]:
                syms[namespace]["func"].append(symb)
            elif symtype in ["s"]:
                syms[namespace]["struct"].append(symb)
            elif symtype in ["g"]:
                syms[namespace]["enum"].append(symb)
            elif symtype in ["d"]:
                # We don't document our macros
                pass
            else:
                logging.error("Unhandld symtype: %r", symtype)

    return syms


def find_pp(api):
    """Find pretty-printers"""

    pps = {}

    for symb in api["func"]:
        parts = symb.split("_")
        dtype = "_".join(parts[:-1])
        tail = parts[-1]

        if tail in ["str", "fpr", "pr"]:
            if dtype not in pps:
                pps[dtype] = []
            pps[dtype].append(symb)

    return pps


def emit(namespace, api):
    """Emit an RST"""

    template_loader = jinja2.FileSystemLoader(searchpath="templates")
    template_env = jinja2.Environment(loader=template_loader)
    template = template_env.get_template("api_section.jinja")

    return template.render(
        {
            "ns": namespace,
            "api": api,
            "show_header": os.path.exists(
                os.path.join("..", "..", "..", "include", f"lib{namespace}.h")
            ),
            "show_enums": api["enum"],
            "show_structs": api["struct"],
            "show_funcs": api["func"],
        }
    )


def main(args):
    """Entry point"""

    logging.info("Output: %r", args.output)

    syms = symbols(args)

    def namespace_to_section(namespace):
        for section, namespaces in NAMESPACES.items():
            if namespace in namespaces:
                return section
        return None

    pps = {}
    for namespace, val in syms.items():
        pps[namespace] = find_pp(val)

    for namespace, val in syms.items():
        logging.info("namespace: %r", namespace)

        section = namespace_to_section(namespace)
        if section is None:
            logging.error(
                f"no section for namespace({namespace}); add it to apigen.NAMESPACES"
            )
            return 1

        rst_fpath = os.path.join(args.output, "c", section, "%s.rst" % namespace)
        rst = emit(namespace, val)

        with open(rst_fpath, "w") as rfd:
            rfd.write(rst)

    return 0


if __name__ == "__main__":
    sys.exit(main(setup()))
