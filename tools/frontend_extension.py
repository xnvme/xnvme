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
import subprocess
import sys
import tempfile

PROGRAM = '#!/bin/sh\necho "extension ran with $*"\n'


def section_of(overview, row):
    """The heading of the section of 'overview' that holds the row starting with 'row'"""

    heading = None
    for line in overview.splitlines():
        if line and not line.startswith(" "):
            heading = line
        elif line.strip().startswith(row):
            return heading, line
    return None, None


def write(path, text, mode):
    with open(path, "w", encoding="utf-8") as fd:
        fd.write(text)
    os.chmod(path, mode)


def main(xnvme):
    failed = []

    with tempfile.TemporaryDirectory() as tmp:
        exec_path, ext_dir = (
            os.path.join(tmp, name) for name in ["exec", "extensions"]
        )
        for path in [exec_path, ext_dir]:
            os.mkdir(path, 0o700)
        for name in [
            "extcheck",
            "extcheckstart",
            "extcheckbare",
            "extcheckctrl",
            "test-extcheck",
            "example-extcheck",
            "ext\x1bcheck",
        ]:
            write(os.path.join(exec_path, f"xnvme-{name}"), PROGRAM, 0o700)
        for name, extension, mode in [
            (
                "extcheck",
                "# a comment\nsection=serve\ndescription=Serve a check\n",
                0o644,
            ),
            ("extcheckstart", "section=start\ndescription=Start a check\n", 0o644),
            (
                "extcheckctrl",
                "section=host\ndescription=\x1b[31mRed\x07 \u009b2Jcheck\n",
                0o644,
            ),
        ]:
            write(os.path.join(ext_dir, f"{name}.extension"), extension, mode)
        env = dict(
            os.environ,
            XNVME_EXEC_PATH=exec_path,
            XNVME_EXTENSION_DIR=ext_dir,
        )

        def run(*args):
            return subprocess.run(
                [xnvme, *args],
                env=env,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )

        overview = run().stdout
        for row, heading, descr in [
            ("xnvme extcheck ", "Serve devices:", "Serve a check"),
            ("xnvme extcheckstart ", "Start here:", "Start a check"),
            ("xnvme extcheckctrl ", "Set up the host:", "[31mRed 2Jcheck"),
        ]:
            got, line = section_of(overview, row)
            if got != heading or not line or descr not in line:
                failed.append(f"'{row.strip()}' is not under '{heading}':\n{overview}")
        for ctrl in ["\x1b", "\x07", "\u009b"]:
            if ctrl in overview:
                failed.append(f"the overview has the control character {ctrl!r}")
        if "extcheckbare" in overview:
            failed.append(
                f"the overview lists an extension without a section:\n{overview}"
            )

        listed = run("--list-cmds").stdout
        for name in ["extcheck", "extcheckstart", "extcheckbare"]:
            if name not in listed.split():
                failed.append(f"--list-cmds does not list {name}")
        if "\x1b" in listed:
            failed.append("--list-cmds lists a name with a control character")

        for args, expected in [
            (["extcheck", "--x"], "extension ran with --x"),
            (["test", "extcheck", "--x"], "extension ran with --x"),
            (["example", "extcheck", "--x"], "extension ran with --x"),
        ]:
            ran = run(*args).stdout.strip()
            if ran != expected:
                failed.append(f"xnvme {' '.join(args)} printed '{ran}'")
        for group in ["test", "example"]:
            if f"xnvme {group} extcheck" not in run(group).stdout:
                failed.append(f"xnvme {group} does not list the extension 'extcheck'")

    for msg in failed:
        print(msg)
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1].strip())
        sys.exit(64)
    sys.exit(main(sys.argv[1]))
