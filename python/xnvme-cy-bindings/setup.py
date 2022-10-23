import glob
import os
import re
import shutil
import subprocess
import sysconfig
from pathlib import Path

from setuptools import Extension, find_namespace_packages, setup

try:
    from auxiliary.generate_cython_bindings import generate_pyx
    from Cython.Build import cythonize

    cython = True
except ImportError:
    cython = False

xnvme_init_py_path = "xnvme/__init__.py"
try:
    os.remove(
        xnvme_init_py_path
    )  # Don't misinterpret the local directory as the xnvme package
except FileNotFoundError:
    pass
import xnvme.cython_header  # noqa: E402

PACKAGE_NAME = "xnvme"
cy_xnvme_pkg_path = os.path.dirname(os.path.realpath(__file__))
WORKSPACE_ROOT = cy_xnvme_pkg_path + "/../.."

LIB_PYX = "cython_bindings.pyx"  # NOTE: Has to be named differently than LIBXNVME_PXD to avoid merging namespaces
LIB_PYX_PATH = f"{PACKAGE_NAME}/cython_bindings/{LIB_PYX}"

LIBXNVME_PYX = "libxnvme.pyx"
LIBXNVME_PYX_PATH = f"{PACKAGE_NAME}/cython_bindings/{LIBXNVME_PYX}"
Path(LIBXNVME_PYX_PATH).touch()  # We just need an empty file as build target
LIBXNVME_PXD = "libxnvme.pxd"
shutil.copy(
    xnvme.cython_header.get_include() + "/" + LIBXNVME_PXD,
    f"{PACKAGE_NAME}/cython_bindings/{LIBXNVME_PXD}",
)

with open(xnvme_init_py_path, "w") as f:
    f.write(
        "# This file won't be included in the final package, but it's needed to fix the Cython build.\n"
        "# If this file is not included, the libxnvme module won't have the enum definitions."
    )


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


includedir = "../../include"
libdir = "../../builddir/lib/"
if os.path.isdir(includedir):
    # Assuming installation from Git repo
    with open(LIB_PYX_PATH, "w") as f_out, open(
        xnvme.cython_header.get_include() + "/" + LIBXNVME_PXD, "r"
    ) as pxd_contents:
        f_out.write(generate_pyx(pxd_contents).read())
else:
    # Assuming installation from PyPi package
    os.environ["CFLAGS"] = os.environ.get("CFLAGS", "") + pkg_config("xnvme", "--libs")

    includedir = pkg_config("xnvme", "--variable=includedir")
    assert (
        includedir
    ), "pkg-config returned an empty string as includedir. This won't work."

    libdir = pkg_config("xnvme", "--variable=libdir")
    assert libdir, "pkg-config returned an empty string as libdir. This won't work."

cflag_libraries = [
    x.groups()[0]
    for x in re.finditer(r"-l([a-z]+)", pkg_config("xnvme", "--libs-only-l"))
]

if cython:
    ext_modules = cythonize(
        [
            Extension(
                "xnvme.cython_bindings.cython_bindings",
                sources=[LIB_PYX_PATH],
                libraries=cflag_libraries,
                include_dirs=[includedir],
                # library_dirs=[libdir],
            ),
            Extension(
                "xnvme.cython_bindings.libxnvme",
                sources=[LIBXNVME_PYX_PATH],
                libraries=cflag_libraries,
                include_dirs=[includedir],
                # library_dirs=[libdir],
            ),
        ],
        include_path=[f"{PACKAGE_NAME}/cython_bindings/"],
        annotate=False,
        language_level="3",
    )
else:
    ext_modules = [
        Extension(
            "xnvme.cython_bindings.cython_bindings",
            depends=glob.glob(includedir + "/libxnvme*"),
            sources=[os.path.splitext(LIB_PYX_PATH)[0] + ".c"],
            libraries=cflag_libraries,
            include_dirs=[includedir],
            library_dirs=[libdir],
        ),
        Extension(
            "xnvme.cython_bindings.libxnvme",
            depends=glob.glob(includedir + "/libxnvme*"),
            sources=[os.path.splitext(LIBXNVME_PYX_PATH)[0] + ".c"],
            libraries=cflag_libraries,
            include_dirs=[includedir],
            library_dirs=[libdir],
        ),
    ]

try:
    with open("README.rst", "r") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = ""

EXT_SUFFIX = sysconfig.get_config_var("EXT_SUFFIX")

setup(
    name="xnvme-cy-bindings",
    version="0.6.0",
    author="Mads Ynddal",
    author_email="m.ynddal@samsung.com",
    description="xNVMe Python bindings using Cython",
    long_description=long_description,
    url="https://github.com/OpenMPDK/xNVMe",
    project_urls={
        "Bug Tracker": "https://github.com/OpenMPDK/xNVMe/issues",
    },
    license="Apache License, Version 2.0",
    classifiers=[
        "Programming Language :: Cython",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    data_files=[
        ("bin", glob.glob("bin/*")),
    ],
    packages=find_namespace_packages(include=["xnvme.*"]),
    package_data={
        "": [
            LIB_PYX,
            LIBXNVME_PXD,
            LIBXNVME_PYX,
            "xnvme/cython_bindings/*{EXT_SUFFIX}",
            "requirements.txt",
        ]
    },
    include_package_data=True,
    ext_modules=ext_modules,
    python_requires=">=3.6",
    install_requires=[
        "pytest",
        "setuptools>=58.5",  # Fixes inclusion of libxnvme.pxd in package_data
        "xnvme==0.6.0",
    ],
    setup_requires=["cython"],
    zip_safe=False,
)
