"""
    Definition of xNVMe Python Distribution Package
"""
import codecs
import glob
import os
from setuptools import setup

def read(*parts):
    """Read parts to use a e.g. long_description"""

    here = os.path.abspath(os.path.dirname(__file__))

    # intentionally *not* adding an encoding option to open, See:
    #   https://github.com/pypa/virtualenv/issues/201#issuecomment-3145690
    with codecs.open(os.path.join(here, *parts), 'r') as pfp:
        return pfp.read()

setup(
    name="pyxnvme",
    version="0.0.12",
    description="xNVMe: cross-platform libraries and tools for NVMe devices",
    long_description=read('README.rst'),
    author="Simon A. F. Lund",
    author_email="simon.lund@samsung.com",
#    url="https://github.com/xnvme/xnvme",
    license="Apache License 2.0",
    install_requires=[],
    zip_safe=False,
    packages=["xnvme"],
#    package_dir={"": "modules"},
    data_files=[
        ("bin", glob.glob("bin/*")),
    ],
    options={'bdist_wheel':{'universal':True}},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python",
        "Topic :: Utilities",
        "Topic :: Software Development",
        "Topic :: Software Development :: Testing"
    ],
)
