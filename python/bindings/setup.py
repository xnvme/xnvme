import glob

import setuptools

try:
    with open("README.rst", "r") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = ""

setuptools.setup(
    name="xnvme",
    version="0.7.5",
    author="Simon A. F. Lund",
    author_email="os@safl.dk",
    description="xNVMe language-bindings for Python, through ctypes",
    long_description=long_description,
    url="https://github.com/xnvme/xnvme",
    project_urls={
        "Bug Tracker": "https://github.com/xnvme/xnvme/issues",
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
    packages=["xnvme", "xnvme.tests"],
    python_requires=">=3.9",
    install_requires=[
        "pytest",
    ],
    zip_safe=False,
)
