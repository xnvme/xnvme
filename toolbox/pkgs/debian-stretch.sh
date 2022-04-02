#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Unattended update and upgrade
export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical
apt-get -qy update
apt-get -qy \
  -o "Dpkg::Options::=--force-confdef" \
  -o "Dpkg::Options::=--force-confold" upgrade
apt-get -qy autoclean

# Install packages via apt-get
apt-get install -qy $(cat "toolbox/pkgs/debian-stretch.txt")

# The meson version available via yum is tool old < 0.54 and the Python version is tool old to
# support the next release of meson, so to fix this, Python3 is installed from source and meson
# installed via pip3 / PyPI
wget https://www.python.org/ftp/python/3.7.12/Python-3.7.12.tgz
tar xzf Python-3.7.12.tgz
cd Python-3.7.12
./configure --enable-optimizations
make altinstall -j $(nproc)

# Setup handling of python3 and pip3
update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.7 10
update-alternatives --install /usr/bin/pip3 pip3 /usr/local/bin/pip3.7 10

# Install packages via PyPI
pip3 install meson ninja pyelftools
