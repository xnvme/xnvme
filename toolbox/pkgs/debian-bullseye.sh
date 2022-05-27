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
apt-get -qy install aptitude

# Install packages via the system package-manager (aptitude)
aptitude -q -y -f install $(cat "toolbox/pkgs/debian-bullseye.txt")

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools
