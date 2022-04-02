#!/usr/bin/env python3
"""
    Produce a "bundled" libray that is, a combination of library archives

    The purpose of a library-bundle is to ease the task of linking with a project and
    satisfy its dependencies. For example, libfoo consisting provide its own
    functionality and depends on libx, liby and libz. Instead of users having to link
    with all of foo, x, y and z, then a library-bundle provides a single link-target
    foo.

    The GNU 'ar' tool is used to perform the task on Linux. If the environment variable
    AR_TOOL is set then it is used directly, otherwise the script relies on 'ar' being
    available in PATH. On macOS, libtool will always be used.

    Currently only tested on Linux and macOS, should work on FreeBSD and in the msys
    environment on Windows.
"""
import argparse
import os
import platform
import subprocess
import sys


def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))


def gen_archive_script(bundle_path, lib_paths):
    """Produce ar-script output in 'bundle_path' from 'lib_paths'"""

    ar_input = []
    ar_input.append("CREATE {bundle_path}".format(bundle_path=bundle_path))
    for path in lib_paths:
        ar_input.append("ADDLIB {path}".format(path=path))
    ar_input.append("SAVE")
    ar_input.append("END")

    return "\n".join(ar_input)


def gen_archive(ar_script):
    """Invoke archiver generating library-archive"""

    ar_tool = os.environ.get("AR_TOOL", "ar")
    try:
        with subprocess.Popen(
            [ar_tool, "-M"], stdin=subprocess.PIPE, encoding="ascii"
        ) as proc:
            _, _ = proc.communicate(ar_script)

            return proc.returncode
    except FileNotFoundError as exc:
        return exc.errno

    return 1


def gen_archive_darwin(out_file, libs):
    with subprocess.Popen(
        [
            "/usr/bin/libtool",
            "-static",
            "-o",
            out_file,
            *(expand_path(lpath) for lpath in libs),
        ],
        stdin=subprocess.PIPE,
        encoding="ascii",
    ) as proc:
        _, _ = proc.communicate()
    return proc.returncode


def parse_args():
    """Parse arguments for archiver"""

    prsr = argparse.ArgumentParser(
        description="bundler",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    prsr.add_argument(
        "--output",
        help="Path to directory in which to store runner output",
        default=os.path.join(expand_path("."), "libbundle.a"),
    )
    prsr.add_argument(
        "--libs",
        help="Path to one or more library archives",
        nargs="+",
        required=True,
        type=str,
    )

    return prsr.parse_args()


def main(args):
    """Produce library bundle"""

    if platform.system() == "Darwin":
        res = gen_archive_darwin(args.output, args.libs)
    else:
        ar_script = gen_archive_script(
            args.output, [expand_path(lpath) for lpath in args.libs]
        )
        res = gen_archive(ar_script)

    return res


if __name__ == "__main__":
    sys.exit(main(parse_args()))
