#!/usr/bin/env python3
"""
    Generate man pages for xNVMe CLI tools

    The generator runs each program in 'builddir' with '--help', and parses the output
    to generate man pages: a general page and one for each sub-command. For the xnvme
    front-end it also reads the overview, for the pages of its own commands and groups. The
    tool `txt2man` is required.

    The bash completion is not generated: toolbox/bash_completion.d/xnvme asks xnvme.
"""
from __future__ import print_function

import argparse
import logging
import os
import re
import sys
from pathlib import Path
from subprocess import PIPE, Popen

MESON_MAN_INSTALL = "install_man('{manpage}')"

RE_SIG = "".join(
    [
        r"^Usage:\s(?P<usage>.*)$",
        r"(?P<descr>(?:.|\n)*?)",
        r"(?:(?:Where.*:|With.*:|Positional arguments:)$|(?=^\s+\[))",
        r"(?P<body>(?:.|\n)*?)^See",
    ]
)

MANPAGE_MAIN = """NAME
  ${name} - ${descr}
SYNOPSIS
  ${usage}
DESCRIPTION
  ${descr_long}
${commands}
OPTIONS
  --help
    Print the synopsis and exit

EXAMPLES
  Read the man page for each <command> or consult the command-line --help:

    $ ${name} <command> --help

SEE ALSO
  Full documentation at: <https://xnvme.io/>
AUTHOR
  Written by ${author_name} <${author_email}> on behalf of ${sponsor}
"""

MANPAGE_SUB = """NAME
  ${name} - ${descr}
SYNOPSIS
${usage}
DESCRIPTION
  ${descr_long}
${required}
${optional}

SEE ALSO
  Full documentation at: <https://xnvme.io/>
AUTHOR
  Written by ${author_name} <${author_email}> on behalf of ${sponsor}
"""


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def run(cmd, cmd_input=None, cwd=None):
    """Run the given 'cmd' and return (stdout, stderr, returncode)"""

    with Popen(
        " ".join(cmd),
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        cwd=cwd,
        shell=True,
        env={"PATH": cwd, "XNVME_EXEC_PATH": os.devnull} if cwd else None,
    ) as proc:
        out, err = proc.communicate(
            input=cmd_input.encode("utf-8") if cmd_input else None
        )
        rcode = proc.returncode

        return out.decode("utf-8"), err.decode("utf-8"), rcode


# REUSE-IgnoreStart
def emit_meson_build(args, folder, meson_file):
    """Emit a meson.build file"""
    with open(os.sep.join([args.output, folder, "meson.build"]), "w") as mfd:
        mfd.write("# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd\n")
        mfd.write("#\n")
        mfd.write("# SPDX-License-Identifier: BSD-3-Clause\n")
        mfd.write("\n")
        mfd.write("\n".join(meson_file))


# REUSE-IgnoreEnd


def parse_tool_sub_sig(tsig, sname):
    """Parse the signature of the given tool sub-command, from the --help of xnvme_cli"""

    tname = tsig["name"]

    out, err, rcode = run([tname, sname, "--help"], cwd=tsig["dirname"])
    if rcode:
        logging.error(out, err, rcode)
        return False

    match = re.match(RE_SIG, str(out), re.MULTILINE | re.IGNORECASE)
    if not match:
        logging.error("No match!")
        return False

    sig = {
        "name": sname,
        "usage": "  " + match.group("usage"),
        "descr": match.group("descr").strip(),
        "descr_long": match.group("descr").strip(),
        "args": [],
    }

    for line in match.group("body").strip().splitlines():
        try:
            arg, descr = line.strip().split(";", 1)
        except ValueError:
            # skip headings and newlines in body e.g. "With <args> for backend:"
            continue

        arg = arg.strip()
        if arg[0] not in ["-", "["]:
            arg = "<%s>" % arg

        sig["args"].append({"arg": arg, "descr": descr.strip()})

    tsig["subs"][sname] = sig

    return True


def prose(text):
    """Text for a man page section: paragraphs reflowed, indented ones such as tables kept"""

    paras = []
    for para in text.strip().split("\n\n"):
        lines = para.splitlines()
        if any(line.startswith(" ") for line in lines):
            paras.append("\n  ".join(lines))
        else:
            paras.append(" ".join(lines))

    return "\n\n  ".join(paras)


def summary(text):
    """The first sentence, or clause, of a description: the one-liner of a man page NAME"""

    return re.split(r"(?<=[.;])\s", text.strip(), maxsplit=1)[0].rstrip(".;")


def parse_frontend_sub_sig(tsig, words, descr):
    """
    Parse the signature of a command the xnvme front-end implements itself: its --help is
    the usage, a blank line, and prose naming the options
    """

    out, _, rcode = run([tsig["name"], *words, "--help"], cwd=tsig["dirname"])
    if rcode or not out.startswith("Usage:"):
        return None

    # A command of xnvme_cli, such as 'info', is documented from its own signature
    match = re.match(RE_SIG, out, re.MULTILINE | re.IGNORECASE)
    if match:
        sname = match.group("usage").split()[1]
        if sname not in tsig["subs"] and parse_tool_sub_sig(tsig, sname):
            return sname
        return None

    # The lines after the first are indented to line up under it, past 'Usage: '
    usage, _, body = out.partition("\n\n")
    sname = "-".join(words)
    tsig["subs"][sname] = {
        "name": sname,
        "usage": "\n".join(
            "  " + line[len("Usage: ") :] for line in usage.splitlines()
        ),
        "descr": descr,
        "descr_long": prose(body) or descr,
        "args": [],
    }

    return sname


def parse_frontend(tsig):
    """
    Parse the commands listed by the xnvme front-end's overview, and those of its groups;
    returns their names, or none for a tool that is not the front-end
    """

    out, _, rcode = run([tsig["name"]], cwd=tsig["dirname"])
    if rcode or "Groups, run one" not in out:
        return []

    snames = []
    for line in out.splitlines():
        match = re.match(r"^  xnvme (\w[\w-]*)(?: <\w+>)?\s{2,}(.+)$", line)
        if not match or match.group(1) == "help":
            continue
        name, descr = match.groups()

        # A group lists its commands with a description each; programs are documented apart
        lst, _, rcode = run([tsig["name"], name, "--help"], cwd=tsig["dirname"])
        if not rcode and lst.startswith("Commands in"):
            nested = f"See 'xnvme {name} <command>" in lst
            for row in re.findall(r"^  (\S+)\s{2,}(.+)$", lst, re.MULTILINE):
                words = [name, row[0]] if nested else [row[0]]
                sname = parse_frontend_sub_sig(tsig, words, row[1])
                if sname:
                    snames.append(sname)
            continue

        sname = parse_frontend_sub_sig(tsig, [name], descr)
        if sname:
            snames.append(sname)

    return snames


def parse_tool_sig(tpath):
    """Parse the signature of the given tool and its sub-commands"""

    tname = os.path.basename(tpath)
    tdirname = os.path.dirname(tpath)

    if not tdirname:  # Expect to find it in $PATH
        for path in os.getenv("PATH").split(":"):
            if os.path.exists(os.path.join(path, tname)):
                tdirname = path
                break

    tdirname = expand_path(tdirname)
    logging.info("Using tool at: %s", tdirname)

    out, err, rcode = run([tname, "--help"], cwd=tdirname)
    if rcode:
        logging.error(out, err, rcode)
        return None

    if "\n\nStart here:\n" in out:
        # The front-end's --help is its overview: the usage, a description, then the command
        # tree; the commands of xnvme_cli are among those 'xnvme --list-cmds' prints
        usage, descr = out.split("\n\n")[:2]
        usage = usage[len("Usage: ") :]
        lst, _, _ = run([tname, "--list-cmds"], cwd=tdirname)
        names = lst.split()
    else:
        match = re.match(RE_SIG, out, re.MULTILINE | re.IGNORECASE)
        if not match:
            logging.error("No match!")
            return None
        usage, descr = match.group("usage"), match.group("descr")
        names = [
            line.split()[0] for line in match.group("body").splitlines() if line.strip()
        ]

    tsig = {
        "name": tname,
        "path": tpath,
        "dirname": tdirname,
        "usage": usage,
        "descr": summary(descr),
        "descr_long": prose(descr),
        "snames": [],
        "subs": {},
    }

    tsig["snames"] = parse_frontend(tsig)

    for sname in names:
        if sname in tsig["subs"]:
            continue

        if not parse_tool_sub_sig(tsig, sname):
            logging.warning("skipping tname: %s, sname: %s", tname, sname)
            continue
        tsig["snames"].append(sname)

    return tsig


def emit_manpage_sub(tool, sub):
    """Emit man page for the given tool sub-command"""

    required = []
    optional = []

    for arg in sub["args"]:
        arg_txt = "  %s  %s\n\n" % (arg["arg"], arg["descr"])
        if arg["arg"][0] in ["<", "-"]:
            required.append(arg_txt)
        elif arg["arg"][0] in ["["]:
            optional.append(arg_txt)
        else:
            return None

    txtpage = (
        MANPAGE_SUB.replace("${name}", "-".join([tool["name"], sub["name"]]))
        .replace("${descr}", sub["descr"] if sub["descr"] else "None provided")
        .replace(
            "${descr_long}", sub["descr_long"] if sub["descr_long"] else "None provided"
        )
        .replace("${usage}", sub["usage"])
        .replace("${required}", "REQUIRED\n" + "".join(required) if required else "")
        .replace("${optional}", "OPTIONAL\n" + "".join(optional) if optional else "")
        .replace("${author_name}", "Simon A. F. Lund")
        .replace(
            "${author_email}",
            "simon.lund@samsung.com",
        )
        .replace("${sponsor}", "Samsung")
    )

    title = "-".join([tool["name"].upper(), sub["name"].upper()])
    manpage, err, rcode = run(
        [
            "txt2man",
            "-t",
            title,
            "-v",
            "xNVMe",
            "-s",
            "1",
            "-r",
            "xNVMe",
        ],
        txtpage,
    )
    if rcode:
        logging.error("FAILED: txt2man; %s, %d", err, rcode)
        return None

    return manpage, txtpage


def emit_manpage_main(tool):
    """Emit man page for the given tool"""

    txt_subs = "COMMANDS\n"

    for sname in tool["snames"]:
        txt_subs += "  %s-%s(1)" % (tool["name"], sname)
        txt_subs += "  %s\n\n" % tool["subs"][sname]["descr"]

    txtpage = (
        MANPAGE_MAIN.replace("${name}", tool["name"])
        .replace(
            "${descr}",
            tool["descr"] if tool["descr"] else "No short description",
        )
        .replace("${usage}", tool["usage"])
        .replace("${commands}", txt_subs)
        .replace("${author_name}", "Simon A. F. Lund")
        .replace(
            "${author_email}",
            "simon.lund@samsung.com",
        )
        .replace("${sponsor}", "Samsung")
        .replace(
            "${descr_long}",
            tool["descr_long"] if tool["descr_long"] else "No long description",
        )
    )

    title = tool["name"].upper()
    manpage, err, rcode = run(
        [
            "txt2man",
            "-t",
            title,
            "-v",
            "xNVMe",
            "-s",
            "1",
            "-r",
            "xNVMe",
        ],
        txtpage,
    )
    if rcode:
        logging.error("FAILED: txt2man; '%s', rcode:%s", err, rcode)
        return None

    return manpage, txtpage


def gen_manpage(args, folders):
    """Generate man pages"""

    for folder, tools in folders.items():
        meson = []  # Populate with lines for a meson.build file

        logging.info("Writing man pages to: %r", args.output)
        for tool in tools:
            tool_fname = "%s.1" % tool["name"]
            tool_fpath = Path(args.output) / folder / tool_fname
            tool_fpath_txt = Path("/tmp") / f"{tool_fname}.txt"

            logging.info(f"Generating for {tool['name']} at {tool_fpath}")

            manpage, txtpage = emit_manpage_main(tool)
            if manpage is None:
                return 1

            with tool_fpath.open("w") as tfd:
                tfd.write(manpage)

            with tool_fpath_txt.open("w") as txt:
                txt.write(txtpage)

            meson.append(MESON_MAN_INSTALL.format(manpage=tool_fname))

            for sname in tool["snames"]:
                sub = tool["subs"][sname]
                sub_fname = "%s-%s.1" % (tool["name"], sub["name"])
                sub_fpath = Path(args.output) / folder / sub_fname
                sub_fpath_txt = Path("/tmp") / f"{sub_fname}.txt"

                logging.info("Generating '%s'", sub_fpath)

                manpage, txtpage = emit_manpage_sub(tool, sub)
                with sub_fpath.open("w") as mfd:
                    mfd.write(manpage)

                with sub_fpath_txt.open("w") as txt:
                    txt.write(txtpage)

                meson.append(MESON_MAN_INSTALL.format(manpage=sub_fname))

        meson.sort()

        emit_meson_build(args, folder, meson)

    return 0


def find_binaries(folder):
    """Return the paths of the executables in builddir, run from there since installed
    ones are off PATH"""

    builddir = Path(__file__).parent.parent / "builddir"

    bins = []

    # These examples parse argv by hand, so their --help is not one to generate from
    skip = ("dev", "enum", "hello", "single-")
    for path in sorted((builddir / folder).rglob("*")):
        if path.stem.startswith(tuple(f"xnvme-example-{name}" for name in skip)):
            continue
        if path.is_file() and path.stat().st_mode & os.X_OK:
            bins.append(str(path))

    return bins


def setup():
    """Parse command-line arguments for generator and setup logger"""

    generators = {"man": gen_manpage}

    prsr = argparse.ArgumentParser(
        description="xNVMe CLI man page generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "generator",
        help="Generator to run",
        default=sorted(generators.keys())[0],
        choices=sorted(generators.keys()),
    )
    prsr.add_argument(
        "--output",
        help="Path to directory in which to emit subfolders with man pages",
        default=os.sep.join(["."]),
    )
    prsr.add_argument(
        "--log-level",
        help="log-devel",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
    )

    args = prsr.parse_args()
    args.output = expand_path(args.output)
    args.gen = generators[args.generator]

    logging.basicConfig(
        format="%(asctime)s %(message)s",
        level=getattr(logging, args.log_level.upper(), None),
    )

    return args


def main(args):
    """Generate man pages for xNVMe CLI tools"""

    folders = {"examples": [], "tests": [], "tools": []}
    for folder, tools in folders.items():
        for tool in find_binaries(folder):
            logging.info("Parsing tool: %r", tool)

            tsig = parse_tool_sig(tool)
            if not tsig or not tsig.get("snames", None):
                logging.error("failed parsing snames from tool: '%s'", tool)
                continue

            tools.append(tsig)

    return args.gen(args, folders)


if __name__ == "__main__":
    sys.exit(main(setup()))
