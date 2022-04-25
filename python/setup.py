import codecs
import glob
import os

import setuptools


def read(*parts):
    """Read parts to use a e.g. long_description"""

    here = os.path.abspath(os.path.dirname(__file__))

    # intentionally *not* adding an encoding option to open, See:
    #   https://github.com/pypa/virtualenv/issues/201#issuecomment-3145690
    with codecs.open(os.path.join(here, *parts), "r") as pfp:
        return pfp.read()


setuptools.setup(
    name="xnvme",
    version="0.3.0",
    author="Simon A. F. Lund",
    author_email="os@safl.dk",
    description="xNVMe Cython and ctypes language-bindings for Python",
    long_description=read("README.rst"),
    url="https://github.com/OpenMPDK/xNVMe",
    project_urls={
        "Bug Tracker": "https://github.com/OpenMPDK/xNVMe/issues",
    },
    license="Apache License, Version 2.0",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    data_files=[
        ("bin", glob.glob("bin/*")),
    ],
    package_dir={"": "./"},
    packages=setuptools.find_packages(where="./"),
    python_requires=">=3.6",
    install_requires=["pytest"],
)
