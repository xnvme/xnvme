#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Checks that xnvme lists and runs an extension, a program xnvme-<name> that xNVMe does not
install, in the section of the overview that its <name>.extension names, and that it neither
runs a program nor reads a .extension that another user can change

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
        exec_path, open_path, ext_dir = (
            os.path.join(tmp, name) for name in ["exec", "open", "extensions"]
        )
        for path in [exec_path, open_path, ext_dir]:
            os.mkdir(path, 0o700)
        for name in [
            "extcheck",
            "extcheckstart",
            "extcheckbare",
            "extcheckctrl",
            "extcheckopenext",
            "test-extcheck",
            "example-extcheck",
            "ext\x1bcheck",
        ]:
            write(os.path.join(exec_path, f"xnvme-{name}"), PROGRAM, 0o700)
        write(os.path.join(exec_path, "xnvme-extcheckopen"), PROGRAM, 0o707)
        write(os.path.join(open_path, "xnvme-extcheckopendir"), PROGRAM, 0o700)
        os.chmod(open_path, 0o707)
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
            ("extcheckopenext", "section=host\ndescription=Open check\n", 0o646),
        ]:
            write(os.path.join(ext_dir, f"{name}.extension"), extension, mode)
        env = dict(
            os.environ,
            XNVME_EXEC_PATH=f"{exec_path}:{open_path}",
            XNVME_EXTENSION_DIR=ext_dir,
        )
        env.pop("SUDO_UID", None)

        def run(*args, **extra):
            return subprocess.run(
                [xnvme, *args],
                env=dict(env, **extra),
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )

        shown = run()
        overview = shown.stdout
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
        if "extcheckopenext" in overview or "not reading" not in shown.stderr:
            failed.append(
                f"the overview reads a .extension every user can write:\n{shown}"
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

        for name in ["extcheckopen", "extcheckopendir"]:
            refused = run(name)
            if (
                refused.returncode == 0
                or "writable by every user" not in refused.stderr
            ):
                failed.append(
                    f"xnvme {name} ran a program every user can change:\n{refused}"
                )

        # Changing the owner of a file takes root, as the Linux containers of CI run the tests
        if os.geteuid() == 0:
            other = os.path.join(exec_path, "xnvme-extcheck")
            os.chown(other, 65534, -1)
            refused = run("extcheck")
            if refused.returncode == 0 or "owned by uid 65534" not in refused.stderr:
                failed.append(f"xnvme ran a program of another user:\n{refused}")
            ran = run("extcheck", SUDO_UID="65534").stdout.strip()
            if ran != "extension ran with":
                failed.append(
                    f"xnvme did not run a program of the user who ran sudo: '{ran}'"
                )

    for msg in failed:
        print(msg)
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1].strip())
        sys.exit(64)
    sys.exit(main(sys.argv[1]))
