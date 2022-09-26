#!/usr/bin/env python3
"""
    Generate Bash-completions and man pages for xNVMe CLI tools

    The generator requires that the tools are installed on the system, since it runs the
    tool to parse the '--help' output in order to generate the corresponding
    bash-completion script and man page.

    Bash-completions) One bash-completion script is generated per CLI tool

    man pages) multiple man-pages are generated per CLI tool, a general page and one for
    each sub-command. The tool `txt2man` is required for the man generator
"""
from __future__ import print_function

import argparse
import logging
import os
import re
import sys
from subprocess import PIPE, Popen

MESON_BASH_INSTALL = (
    "install_data('{completion}', "
    "install_dir: bash_completion_dep.get_variable('completionsdir'))"
)
MESON_MAN_INSTALL = "install_man('{manpage}')"

RE_SIG = "".join(
    [
        r"^Usage:\s(?P<usage>.*)$",
        r"(?P<descr>(.|\n)*)",
        r"Where.*:$\n\n(?P<body>(.|\n)*)\n\nSee",
    ]
)

SNAMES = """
    "${sname}")
        opts+="${opts}"
        ;;
"""

SCRIPT = """# ${tname} completion                           -*- shell-script -*-
#
# Bash completion script for the `${tname}` CLI
#
# Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
# SPDX-License-Identifier: Apache-2.0

_${tname}_completions()
{
    local cur=${COMP_WORDS[COMP_CWORD]}
    local sub=""
    local opts=""

    COMPREPLY=()

    # Complete sub-commands
    if [[ $COMP_CWORD < 2 ]]; then
        COMPREPLY+=( $( compgen -W '${snames} --help' -- $cur ) )
        return 0
    fi

    # Complete sub-command arguments

    sub=${COMP_WORDS[1]}

    if [[ "$sub" != "enum" ]]; then
        opts+="/dev/nvme* "
    fi

    case "$sub" in
    ${subs}
    esac

    COMPREPLY+=( $( compgen -W "$opts" -- $cur ) )

    return 0
}

#
complete -o nosort -F _${tname}_completions ${tname}

# ex: filetype=sh
"""

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
        " ".join(cmd) if cwd else cmd,
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        cwd=cwd,
        shell=True,
        env={"PATH": cwd} if cwd else None,
    ) as proc:
        out, err = proc.communicate(
            input=cmd_input.encode("utf-8") if cmd_input else None
        )
        rcode = proc.returncode

        return out.decode("utf-8"), err.decode("utf-8"), rcode


def parse_tool_sub_sig(tsig, sname):
    """Parse the signature of the given tool sub-command"""

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
        "usage": match.group("usage"),
        "descr": match.group("descr").strip(),
        "args": [],
        "opts": [],
    }

    for line in match.group("body").strip().splitlines():
        arg, descr = line.strip().split(";", 1)

        arg = arg.strip()
        if arg[0] not in ["-", "["]:
            arg = "<%s>" % arg

        sig["args"].append({"arg": arg, "descr": descr.strip()})

        if "--" in arg:
            opt = arg.replace("[", "").replace("]", "").strip().split(" ")[0]
            sig["opts"].append(opt)

    tsig["subs"][sname] = sig

    return True


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

    match = re.match(RE_SIG, out, re.MULTILINE | re.IGNORECASE)
    if not match:
        logging.error("No match!")
        return None

    tsig = {
        "name": tname,
        "path": tpath,
        "dirname": tdirname,
        "usage": match.group("usage"),
        "descr": match.group("descr").strip(),
        "descr_long": match.group("descr").strip(),
        "snames": [],
        "subs": {},
    }

    for line in match.group("body").splitlines():
        sname = line.strip().split(" ")[0]
        tsig["snames"].append(sname)

        if not parse_tool_sub_sig(tsig, sname):
            logging.error("FAILED: parsing tname: %s, sname: %s", tname, sname)
            return None

    return tsig


def emit_completion(tool):
    """Emits completion script for the given tool-struct"""

    cases = ""
    for sname in tool["snames"]:
        cases += SNAMES.replace("${sname}", sname).replace(
            "${opts}", " ".join(tool["subs"][sname]["opts"])
        )

    compl = (
        SCRIPT.replace("${tname}", tool["name"])
        .replace("${snames}", " ".join(tool["snames"]))
        .replace("${subs}", cases)
    )

    return compl


def gen_completions(args, tools):
    """Generate Bash-completions"""

    meson = []  # Populate with lines for a meson.build file

    logging.info("Writing scripts to: %r", args.output)
    for tool in tools:

        tool_fname = "%s-completions" % tool["name"]
        tool_fpath = os.sep.join([args.output, tool_fname])

        logging.info("Generating for %r at %r", tool["name"], tool_fpath)

        script = emit_completion(tool)

        with open(tool_fpath, "w") as tfd:
            tfd.write(script)

        meson.append(MESON_BASH_INSTALL.format(completion=tool_fname))

    meson.sort()

    # Emit a meson.build
    with open(os.sep.join([args.output, "meson.build"]), "w") as mfd:
        mfd.write("\n".join(meson))

    return 0


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
        .replace("${descr_long}", sub["descr"] if sub["descr"] else "None provided")
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

    manpage, err, rcode = run(
        [
            "txt2man",
            "-t",
            "-".join([tool["name"].upper(), sub["name"].upper()]),
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

    return manpage


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

    manpage, err, rcode = run(
        [
            "txt2man",
            "-t",
            tool["name"].upper(),
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

    return manpage


def gen_manpage(args, tools):
    """Generate man pages"""

    meson = []  # Populate with lines for a meson.build file

    logging.info("Writing man pages to: %r", args.output)
    for tool in tools:
        tool_fname = "%s.1" % tool["name"]
        tool_fpath = os.sep.join([args.output, tool_fname])

        logging.info("Generating for %r at %r", tool["name"], tool_fpath)

        manpage = emit_manpage_main(tool)
        if manpage is None:
            return 1

        with open(tool_fpath, "w") as tfd:
            tfd.write(manpage)

        meson.append(MESON_MAN_INSTALL.format(manpage=tool_fname))

        for sname in tool["snames"]:
            sub = tool["subs"][sname]
            sub_fname = "%s-%s.1" % (tool["name"], sub["name"])
            sub_fpath = os.sep.join([args.output, sub_fname])

            logging.info("Generating '%s'", sub_fpath)

            manpage = emit_manpage_sub(tool, sub)
            with open(sub_fpath, "w") as mfd:
                mfd.write(manpage)

            meson.append(MESON_MAN_INSTALL.format(manpage=sub_fname))

    meson.sort()

    # Emit a meson.build
    with open(os.sep.join([args.output, "meson.build"]), "w") as mfd:
        mfd.write("\n".join(meson))

    return 0


def setup():
    """Parse command-line arguments for generator and setup logger"""

    generators = {"man": gen_manpage, "cpl": gen_completions}

    prsr = argparse.ArgumentParser(
        description="xNVMe CLI Bash-completions and man page generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "generator",
        help="Generator to run",
        default=sorted(generators.keys())[0],
        choices=sorted(generators.keys()),
    )
    prsr.add_argument(
        "--tools",
        nargs="+",
        required=True,
        help="Name of tools to generate bash-completions for",
    )
    prsr.add_argument(
        "--output",
        help="Path to directory in which to emit completion scripts",
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
    """Generate bash-completions and man-pages for xNVMe CLI tools"""

    tools = []
    for tool in args.tools:  # Parse tools, their subs and args
        logging.info("Parsing tool: %r", tool)

        tsig = parse_tool_sig(tool)
        if not tsig["snames"]:
            logging.error("failed parsing snames from tool: '%s'", tool)

        tools.append(tsig)

    return args.gen(args, tools)


if __name__ == "__main__":
    sys.exit(main(setup()))
