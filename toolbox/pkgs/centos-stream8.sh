#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# This repos has CUnit-devel
dnf config-manager --set-enabled powertools

# Install packages via dnf
dnf install -y $(cat "toolbox/pkgs/centos-stream8.txt")

# The meson version available via yum is tool old < 0.54 and the Python version is tool old to
# support the next release of meson, so to fix this, Python3 is installed from source and meson
# installed via pip3 / PyPI
wget https://www.python.org/ftp/python/3.7.12/Python-3.7.12.tgz
tar xzf Python-3.7.12.tgz
cd Python-3.7.12
./configure --enable-optimizations
make altinstall -j $(nproc)

# Setup handling of python3 and pip3
ln -s /usr/local/bin/pip3.7 /usr/local/bin/pip3
ln -s /usr/local/bin/python3.7 /usr/local/bin/python3

pip3 install meson ninja pyelftools
