#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Checks that xnvme lists and runs a plugin, a program xnvme-<name> that xNVMe does not install

    frontend_plugin.py XNVME
"""
import os
import stat
import subprocess
import sys
import tempfile


def main(xnvme):
    with tempfile.TemporaryDirectory() as exec_path:
        plugin = os.path.join(exec_path, "xnvme-plugincheck")
        with open(plugin, "w") as fd:
            fd.write('#!/bin/sh\necho "plugin ran with $*"\n')
        os.chmod(plugin, stat.S_IRWXU)
        env = dict(os.environ, XNVME_EXEC_PATH=exec_path)

        def run(*args):
            return subprocess.run(
                [xnvme, *args], env=env, capture_output=True, text=True
            )

        failed = []
        overview = run().stdout
        if "Plugins" not in overview or "xnvme plugincheck" not in overview:
            failed.append(f"the overview does not list the plugin:\n{overview}")
        if "plugincheck" not in run("--list-cmds").stdout.split():
            failed.append("--list-cmds does not list the plugin")
        ran = run("plugincheck", "--x").stdout.strip()
        if ran != "plugin ran with --x":
            failed.append(f"xnvme plugincheck --x printed '{ran}'")

    for msg in failed:
        print(msg)
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1].strip())
        sys.exit(64)
    sys.exit(main(sys.argv[1]))
