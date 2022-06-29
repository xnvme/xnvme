import os
import re
import subprocess

from Cython.Build import cythonize
from setuptools import Extension, setup

import xnvme.cython_header  # isort:skip


def pkg_config(*args):
    with subprocess.Popen(
        ["pkg-config", *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ) as proc:
        stdout, _ = proc.communicate()
        if proc.returncode == 0:
            return stdout.decode("utf-8").strip()
        return ""


os.environ["CFLAGS"] = os.environ.get("CFLAGS", "") + pkg_config("xnvme", "--libs")

includedir = pkg_config("xnvme", "--variable=includedir")
assert includedir, "pkg-config returned an empty string as includedir. This won't work."

libdir = pkg_config("xnvme", "--variable=libdir")
assert libdir, "pkg-config returned an empty string as libdir. This won't work."

setup(
    name="test_cython",
    ext_modules=cythonize(
        [
            Extension(
                "test_cython",
                sources=["test_cython.pyx"],
                libraries=[
                    x.groups()[0]
                    for x in re.finditer(
                        r"-l([a-z]+)", pkg_config("xnvme", "--libs-only-l")
                    )
                ],
                include_dirs=[includedir],
                library_dirs=[libdir],
            ),
        ],
        include_path=[xnvme.cython_header.get_include()],
        language_level="3",
        compiler_directives={
            "binding": True,  # INFO: Important to set this!!!
        },
    ),
    zip_safe=False,
    tests_require=["pytest-cython-collect"],
)
