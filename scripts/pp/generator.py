#!/usr/bin/env python3
"""
    Generate pretty-print helper-functions _yaml, _fpr, _pr, _str for enums and structs.
"""
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import logging
import sys
import os
try:
    import jinja2
except ImportError:
    print("Please install jinja2")
    sys.exit(1)
try:
    import yaml
except Importerror:
    print("Please install yaml")
    sys.exit(1)

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

def tags_from_file(path):
    """Emit tags from the tag-file at the given 'path'"""

    with open(path) as cfd:
        for line in cfd.readlines():
            tag = [x.strip() for x in line.split("\t")]
            if len(tag) <= 3:   # Skip comments
                continue

            yield tag

def declr_has_sym(declr, sym):
    """Check if the given declr has sym"""

    for existing in declr:
        if existing["kind"] == sym["kind"] and existing["name"] == sym["name"]:
            return True

    return False

def tags_to_declr(args, tags):
    """Returns symbols for, and grouped by, the given namespaces"""

    fname = os.path.basename(args.hdr_file)
    name = os.path.splitext(fname)[0]

    hdr_fname = "%s_pp.h" % name
    hdr_name = "%s_PP" % name.upper()
    hdr_path = os.path.join(args.hdr_output, hdr_fname)

    src_fname = "%s_pp.c" % name.replace("lib", "")
    src_path = os.path.join(args.src_output, src_fname)

    declr = []

    for tag in tags:
        sym = {
            "name": tag[0].strip(),
            "name_short": tag[0].strip(),
            "members": [],
            "file": tag[1].strip(),
            "kind": next(iter([k for k in tag[2:] if len(k) == 1]), None),
            "ns": None
        }
        if sym["kind"] is None:
            logging.error("Failed determining kind for '%s'", sym["name"])
            continue

        sym["ns"] = next(
            iter([n for n in args.namespaces if sym["name"].lower().startswith(n)]),
            None
        )
        if sym["ns"] and sym["name"].lower().startswith(sym["ns"]):
            sym["name_short"] = sym["name"][len(sym["ns"]) + 1:]

        if sym["kind"] in ["p", "f", "d", "s", "g"]:
            if args.namespaces and not sym["ns"]:
                logging.info("Skipping, not in ns")
                continue

            if not declr_has_sym(declr, sym):
                declr.append(sym)

        elif sym["kind"] in ["e"]: # Enumeration member / value
            field, ename = [k.strip() for k in tag[-1].split(":")]
            if field != "enum":
                continue

            if args.namespaces and not [n for n in args.namespaces if ename.lower().startswith(n)]:
                continue

            parent = {"name": ename, "members": [], "file": sym["file"], "kind": "g"}
            if not declr_has_sym(declr, parent):
                declr.append(parent)

            for parent in declr:
                if parent["kind"] == "g" and parent["name"] == ename:
                    parent["members"].append((sym["name"], sym["name_short"]))

        elif sym["kind"] in ["m"]: # Structure and union members
            logging.error("Unhandled member-sym_type: %r", sym["kind"])

        else:
            logging.error("Unhandled sym_type: %r", sym["kind"])

    return {
        "ents": declr,

        "fname": fname,
        "name": name,

        "hdr_fname": hdr_fname,
        "hdr_name": hdr_name,
        "hdr_path": hdr_path,

        "src_fname": src_fname,
        "src_path": src_path,
    }

def setup():
    """Parse command-line arguments for generator and setup logger"""

    prsr = argparse.ArgumentParser(
        description="xNVMe pp-generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        "--hdr-file",
        required=True,
        help="Path to the header-filer to generate pp-helpers for"
    )
    prsr.add_argument(
        "--namespaces",
        nargs='+',
        default=[],
        help="Filter objects by their C namespaces e.g. 'xnvme_cmd', 'xnvme_znd'",
    )
    prsr.add_argument(
        "--fmt",
        help="Path to the format declaration to use",
        default=os.sep.join([".", "xnvme_spec.yml"])
    )
    prsr.add_argument(
        "--templates",
        required=True,
        help="Path to directory containing templates"
    )
    prsr.add_argument(
        "--hdr-output",
        help="Path to directory in which to emit pp-header-file",
        default=os.sep.join(["."])
    )
    prsr.add_argument(
        "--src-output",
        help="Path to directory in which to emit pp-source-file",
        default=os.sep.join(["."])
    )
    prsr.add_argument(
        "--log-level",
        help="log-devel",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
    )

    args = prsr.parse_args()
    args.fmt = expand_path(args.fmt)
    args.hdr_file = expand_path(args.hdr_file)
    args.templates = expand_path(args.templates)
    args.hdr_output = expand_path(args.hdr_output)
    args.src_output = expand_path(args.src_output)

    logging.basicConfig(
        format='%(asctime)s %(message)s',
        level=getattr(logging, args.log_level.upper(), None)
    )

    return args

def main(args):
    """Generate pp-helper functions"""

    ctags_fpath = "/tmp/ctags"

    # Run ctags on the given header-file
    _, _, rcode = run([
        "ctags", "-f", ctags_fpath, "--c-kinds=dfpsmge", args.hdr_file
    ])
    if rcode:
        logging.error("Failed running ctags got rcode: %d", rcode)
        return 1

    # Load and convert ctags to declaration-dict
    tags = tags_from_file(ctags_fpath)
    declr = tags_to_declr(args, tags)
    fmt = yaml.load(open(args.fmt), Loader=yaml.Loader)
    fmt["structs"] = [d for d in fmt["structs"] if d["name"].startswith(tuple(args.namespaces))]

    # Generate header
    tmpl_loader = jinja2.FileSystemLoader(searchpath=args.templates)
    tmpl_env = jinja2.Environment(loader=tmpl_loader)

    tmpl_paths = {
        "hdr": "pp-header.jinja",
        "src": "pp-source.jinja"
    }

    with open(declr["hdr_path"], "w") as hfd:
        hfd.write(tmpl_env.get_template(tmpl_paths["hdr"]).render(
            declr=declr, structs=fmt["structs"]
        ))

    with open(declr["src_path"], "w") as hfd:
        hfd.write(tmpl_env.get_template(tmpl_paths["src"]).render(
            declr=declr, structs=fmt["structs"]
        ))

    return 0 if declr else 1

if __name__ == "__main__":
    sys.exit(main(setup()))
