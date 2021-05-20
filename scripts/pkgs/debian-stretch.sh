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

# Install CMake using installer from GitHUB
wget https://github.com/Kitware/CMake/releases/download/v3.16.5/cmake-3.16.5-Linux-x86_64.sh -O cmake.sh
chmod +x cmake.sh
./cmake.sh --skip-license --prefix=/usr/

# Install packages via PyPI
pip3 install meson ninja
