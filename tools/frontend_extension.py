#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Checks that xnvme lists and runs an extension, a program xnvme-<name> that xNVMe does not
install, in the section of the overview that its <name>.extension names

    frontend_extension.py XNVME
"""
import os
import stat
import subprocess
import sys
import tempfile


def section_of(overview, row):
    """The heading of the section of 'overview' that holds the row starting with 'row'"""

    heading = None
    for line in overview.splitlines():
        if line and not line.startswith(" "):
            heading = line
        elif line.strip().startswith(row):
            return heading, line
    return None, None


def main(xnvme):
    with tempfile.TemporaryDirectory() as exec_path, tempfile.TemporaryDirectory() as ext_dir:
        for name, extension in [
            ("extcheck", "# a comment\nsection=serve\ndescription=Serve a check\n"),
            ("extcheckstart", "section=start\ndescription=Start a check\n"),
            ("extcheckbare", None),
        ]:
            prog = os.path.join(exec_path, f"xnvme-{name}")
            with open(prog, "w") as fd:
                fd.write('#!/bin/sh\necho "extension ran with $*"\n')
            os.chmod(prog, stat.S_IRWXU)
            if extension:
                with open(os.path.join(ext_dir, f"{name}.extension"), "w") as fd:
                    fd.write(extension)
        env = dict(os.environ, XNVME_EXEC_PATH=exec_path, XNVME_EXTENSION_DIR=ext_dir)

        def run(*args):
            return subprocess.run(
                [xnvme, *args], env=env, capture_output=True, text=True
            )

        failed = []
        overview = run().stdout
        for row, heading, descr in [
            ("xnvme extcheck ", "Serve devices:", "Serve a check"),
            ("xnvme extcheckstart ", "Start here:", "Start a check"),
        ]:
            got, line = section_of(overview, row)
            if got != heading or not line or descr not in line:
                failed.append(f"'{row.strip()}' is not under '{heading}':\n{overview}")
        if "extcheckbare" in overview:
            failed.append(
                f"the overview lists an extension without a section:\n{overview}"
            )
        cmds = run("--list-cmds").stdout.split()
        for name in ["extcheck", "extcheckstart", "extcheckbare"]:
            if name not in cmds:
                failed.append(f"--list-cmds does not list {name}")
        ran = run("extcheck", "--x").stdout.strip()
        if ran != "extension ran with --x":
            failed.append(f"xnvme extcheck --x printed '{ran}'")

    for msg in failed:
        print(msg)
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1].strip())
        sys.exit(64)
    sys.exit(main(sys.argv[1]))
