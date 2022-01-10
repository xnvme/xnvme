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
apt-get install -qy $(cat "scripts/pkgs/debian-stretch.txt")

# Recent versions of meson needs a recent version of Python.
# The Python3 available via Debian Stretch packages, 3.5, is too old (3.5).
pushd /tmp
PYTHON_VER="3.7.9"
wget "https://www.python.org/ftp/python/${PYTHON_VER}/Python-${PYTHON_VER}.tgz"
tar xzf "Python-${PYTHON_VER}.tgz"
cd "Python-${PYTHON_VER}"
./configure --enable-optimizations
make altinstall -j $(nproc)
update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.7 10
update-alternatives --install /usr/bin/pip3 pip3 /usr/local/bin/pip3.7 10
popd

# Install packages via PyPI
pip3 install meson ninja pyelftools
