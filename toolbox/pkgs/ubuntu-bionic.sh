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
apt-get install -qy $(cat "toolbox/pkgs/ubuntu-bionic.txt")

# Install packages via PyPI
python3 -m pip install --upgrade pip
python3 -m pip install meson ninja pyelftools
