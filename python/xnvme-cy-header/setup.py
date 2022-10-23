import glob

from setuptools import find_namespace_packages, setup

try:
    with open("README.rst", "r") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = ""

setup(
    name="xnvme-cy-header",
    version="0.6.0",
    author="Mads Ynddal",
    author_email="m.ynddal@samsung.com",
    description="Cython header (libxnvme.pxd) for the xNVMe C API",
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
    package_data={"": ["libxnvme.pxd", "test_cython.pyx", "requirements.txt"]},
    include_package_data=True,
    python_requires=">=3.6",
    install_requires=[
        "pytest",
        "setuptools>=40.1",  # Introduction of find_namespace_packages
    ],
    zip_safe=False,
)
