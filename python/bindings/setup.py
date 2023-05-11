import glob

import setuptools

try:
    with open("README.rst", "r") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = ""

setuptools.setup(
    name="xnvme",
    version="0.6.0",
    author="Simon A. F. Lund",
    author_email="os@safl.dk",
    description="xNVMe Cython and ctypes language-bindings for Python",
    long_description=long_description,
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
    packages=setuptools.find_namespace_packages(include=["xnvme.*"]),
    python_requires=">=3.6",
    install_requires=[
        "pytest",
        "setuptools>=40.1",  # Introduction of find_namespace_packages
    ],
    zip_safe=False,
)
