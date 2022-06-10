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
    "macro": [],
    "struct": [],
    "enum": [],
}  # type: dict


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


def symbols(args, namespaces):
    """Returns symbols for, and grouped by, the given namespaces"""

    syms = {}

    with open(args.tags) as cfd:
        for line in cfd.readlines():
            if line[-3] != "\t":
                continue

            symtype = line[-2]
            symb = line.split("\t")[0]

            ns_current = None
            for namespace in namespaces:
                if symb.lower().startswith(namespace):
                    ns_current = namespace
                    break

            if not ns_current:
                continue

            if ns_current not in syms:
                syms[ns_current] = copy.deepcopy(DECLARATIONS)

            if symtype in ["p", "f"]:
                syms[ns_current]["func"].append(symb)
            elif symtype in ["s"]:
                syms[ns_current]["struct"].append(symb)
            elif symtype in ["g"]:
                syms[ns_current]["enum"].append(symb)
            elif symtype in ["d"]:
                syms[ns_current]["macro"].append(symb)
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
                os.path.join("..", "..", "include", f"lib{namespace}.h")
            ),
        }
    )


def main(args):
    """Entry point"""

    logging.info("Output: %r", args.output)

    namespaces = [
        "xnvme_adm",
        "xnvme_dev",
        "xnvme_file",
        "xnvme_geo",
        "xnvme_ident",
        "xnvme_libconf",
        "xnvme_nvm",
        "xnvme_opts",
        "xnvme_spec",
        "xnvme_util",
        "xnvme_ver",
        "xnvme_znd",
        "xnvmec",
        "xnvme",
    ]

    syms = symbols(args, namespaces)

    pps = {}
    for nsprefix, val in syms.items():
        pps[nsprefix] = find_pp(val)

    for nsprefix, val in syms.items():
        logging.info("nsprefix: %r", nsprefix)
        rst_fpath = os.path.join(args.output, "%s.rst" % nsprefix)
        rst = emit(nsprefix, val)

        with open(rst_fpath, "w") as rfd:
            rfd.write(rst)

    return 0


if __name__ == "__main__":
    sys.exit(main(setup()))
