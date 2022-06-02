import glob
import os
from distutils.command.sdist import sdist

from setuptools import setup

PACKAGE_NAME = "xnvme"
cy_xnvme_pkg_path = os.path.dirname(os.path.realpath(__file__))
WORKSPACE_ROOT = cy_xnvme_pkg_path + "/.."
VERSION = "v0.0.0"

LIB_PXD = "libxnvme.pxd"
LIB_PXD_PATH = f"{cy_xnvme_pkg_path}/{PACKAGE_NAME}/{LIB_PXD}"


class pre_process_sdist(sdist):
    def run(self):
        from aux.generate_cython_bindings import (  # Only needed when building
            generate_pxd,
        )

        os.chdir(cy_xnvme_pkg_path)
        with open(LIB_PXD_PATH, "w") as f_out:
            f_out.write(generate_pxd(WORKSPACE_ROOT).read())

        return super().run()


try:
    with open("README.rst", "r") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = ""

setup(
    name=PACKAGE_NAME,
    version=VERSION,
    author="Mads Ynddal",
    author_email="m.ynddal@samsung.com",
    description="xNVMe Cython and ctypes language-bindings for Python",
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
    packages=setuptools.find_namespace_packages(include=["xnvme.*"]) + ["xnvme", "xnvme.tests"],
    package_data={"": [LIB_PXD]},
    include_package_data=True,
    cmdclass={"sdist": pre_process_sdist},
    python_requires=">=3.6",
    install_requires=[
        "pytest",
        "setuptools>=40.1",  # Introduction of find_namespace_packages
    ],
    zip_safe=False,
)
