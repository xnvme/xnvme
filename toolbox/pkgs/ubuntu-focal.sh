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

# Install packages via the system package-manager (apt-get)
apt-get install -qy $(cat "toolbox/pkgs/ubuntu-focal.txt")

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools

# Clone, build and install liburing
git clone https://github.com/axboe/liburing.git
cd liburing
git checkout liburing-2.2
./configure
make
make install
