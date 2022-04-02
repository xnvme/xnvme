#!/usr/bin/env python3
"""
    Produce a binary Debian package from meson builddir

    This has only been tested using meson 0.61, requires the following files from the
    builddir:

    meson-info/intro-installed.json
    meson-info/intro-projectinfo.json

    And the files which 'meson-info/intro-installed.json' refers to.

    NOTE: This is implemented as a quick'n'dirty replacement for the CMake/CPack
    feature.
"""
import argparse
import json
import os
import pathlib
import shutil
import stat
import subprocess
import sys

DEFAULT = {
    "deb_version": "0.0.1",
    "deb_architecture": "amd64",
    "deb_maintainer": "Mr. Robot",
    "deb_description": "Something awesome",
    "deb_package": "noname",
}


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def parse_args():
    """Parse arguments for archiver"""

    prsr = argparse.ArgumentParser(
        description='Create a binary Debian package from "meson-info"',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--builddir", help="Path to meson builddir", required=True, type=str
    )
    prsr.add_argument("--workdir", help="Path to workdir", required=True, type=str)
    prsr.add_argument(
        "--output",
        help="Path to directory in which to store deb-package",
        type=str,
        default=os.path.join(expand_path(".")),
    )

    prsr.add_argument(
        "--deb-package",
        help="Debian control-file property",
    )
    prsr.add_argument(
        "--deb-version",
        help="Debian control-file property",
    )
    prsr.add_argument(
        "--deb-architecture",
        help="Debian control-file property",
    )
    prsr.add_argument(
        "--deb-maintainer",
        help="Debian control-file property",
    )
    prsr.add_argument(
        "--deb-description",
        help="Debian control-file property",
    )

    return prsr.parse_args()


def emit_control_file(args):
    """Emit the Debian package CONTROL file in args.workddir"""

    adict = vars(args)

    pinfo = {}
    with open(
        os.path.join(args.builddir, "meson-info", "intro-projectinfo.json")
    ) as pfd:
        pinfo = json.load(pfd)

    deb_info = DEFAULT
    deb_info["deb_version"] = pinfo.get("version", DEFAULT["deb_version"])
    deb_info["deb_package"] = pinfo.get("descriptive_name", DEFAULT["deb_package"])
    for key, value in DEFAULT.items():
        aval = adict.get(key, None)
        deb_info[key] = value if aval is None else aval

    deb_dir = os.path.join(args.workdir, "DEBIAN")
    os.makedirs(deb_dir)
    with open(os.path.join(deb_dir, "control"), "w") as cfd:
        cfd.write("Package: {deb_package}\n".format(**deb_info))
        cfd.write("Version: {deb_version}\n".format(**deb_info))
        cfd.write("Architecture: {deb_architecture}\n".format(**deb_info))
        cfd.write("Maintainer: {deb_maintainer}\n".format(**deb_info))
        cfd.write("Description: {deb_description}\n".format(**deb_info))


def main(args):
    """Produce library bundle"""

    args.builddir = expand_path(args.builddir)
    args.workdir = expand_path(args.workdir)
    args.output = expand_path(args.output)

    with open(os.path.join(args.builddir, "meson-info", "intro-installed.json")) as ifd:
        install = json.load(ifd)

    emit_control_file(args)

    # Populate the package structure
    for src, dst_install in sorted(install.items()):
        dst = os.path.join(args.workdir, dst_install[1:])

        if not dst.startswith(args.workdir):
            print("Malformat: %s" % dst)
            continue

        dst_dir = os.path.dirname(dst)  # Create directories
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)

        shutil.copyfile(src, dst)  # Copy the file

        if "bin/" in dst:  # Ensure executable
            dst_fd = pathlib.Path(dst)
            dst_fd.chmod(dst_fd.stat().st_mode | stat.S_IEXEC)

    # Run 'dpkg-deb' to create the package
    cmd = [
        "dpkg-deb",
        "--build",
        "--root-owner-group",
        args.workdir,
        args.output,
    ]
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:

        out, err = proc.communicate()
        ret = proc.returncode

    if ret:
        print(out, err)

    return ret


if __name__ == "__main__":
    sys.exit(main(parse_args()))
