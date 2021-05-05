#!/usr/bin/env python3
"""
    Parse the xNVMe header files for auto-generation purposes
"""
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import logging
import pprint
import copy
import sys
import os
import jinja2

DECLARATIONS = {
    "func": [],
    "macro": [],
    "struct": [],
    "enum": [],
}

def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

def run(cmd, cmd_input=None):
    """Run the given 'cmd' and return (stdout, stderr, returncode)"""

    proc = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate(
        input=cmd_input.encode('utf-8') if cmd_input else None
    )
    rcode = proc.returncode

    return out.decode('utf-8'), err.decode('utf-8'), rcode

def setup():
    """Parse command-line arguments for generator and setup logger"""

    prsr = argparse.ArgumentParser(
        description="xNVMe header parser",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        "--tags",
        help="Path to ctags file",
        required=True,
    )
    prsr.add_argument(
        "--output",
        help="Path to directory in which to emit parse-artifacts",
        default=os.sep.join(["."])
    )
    prsr.add_argument(
        "--log-level",
        help="log-devel",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
    )

    args = prsr.parse_args()
    args.tags = expand_path(args.tags)
    args.output = expand_path(args.output)

    logging.basicConfig(
        format='%(asctime)s %(message)s',
        level=getattr(logging, args.log_level.upper(), None)
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

            ns = None
            for namespace in namespaces:
                if symb.lower().startswith(namespace):
                    ns = namespace
                    break

            if not ns:
                continue

            if ns not in syms:
                syms[ns] = copy.deepcopy(DECLARATIONS)

            if symtype in ["p", "f"]:
                syms[ns]["func"].append(symb)
            elif symtype in ["s"]:
                syms[ns]["struct"].append(symb)
            elif symtype in ["g"]:
                syms[ns]["enum"].append(symb)
            elif symtype in ["d"]:
                syms[ns]["macro"].append(symb)
            else:
                logging.error("Unhandld symtype: %r" % symtype)

    return syms

def is_pp(symb):
    """Checks if the given symbol is a pp function"""

    return symb.split("_")[-1] in ["str", "fpr", "pr"]

def find_pp(args, api):
    """Find pretty-printers"""

    pp = {}

    for symb in api["func"]:
        parts = symb.split("_")
        dtype = "_".join(parts[:-1])
        tail = parts[-1]

        if tail in ["str", "fpr", "pr"]:
            if dtype not in pp:
                pp[dtype] = []
            pp[dtype].append(symb)

    return pp

def emit(args, ns, api):
    """Emit an RST"""

    templateLoader = jinja2.FileSystemLoader(searchpath="templates")
    templateEnv = jinja2.Environment(loader=templateLoader)
    template = templateEnv.get_template("api_section.jinja")

    return template.render({"ns": ns, "api": api})

def main(args):
    """Entry point"""

    logging.info("Output: %r", args.output)

    namespaces = [
        "xnvme_3p",
        "xnvme_adm",
        "xnvme_file",
        "xnvme_dev",
        "xnvme_geo",
        "xnvme_nvm",
        "xnvme_sgl",
        "xnvme_spec",
        "xnvme_util",
        "xnvme_ver",
        "xnvme_znd",
        "xnvmec",
        "xnvme"
    ]

    syms = symbols(args, namespaces)

    pps = {}
    for ns in syms:
        pps[ns] = find_pp(args, syms[ns])

    for ns in syms:
        logging.info("ns: %r", ns)
        rst_fpath = os.path.join(args.output, "%s.rst" % ns)
        rst = emit(args, ns, syms[ns])

        with open(rst_fpath, "w") as rfd:
            rfd.write(rst)

    return 0

if __name__ == "__main__":
    sys.exit(main(setup()))
