#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# This repos has CUnit-devel
dnf install -y 'dnf-command(config-manager)'
dnf config-manager --set-enabled crb

# Install packages via the system package-manager (dnf)
dnf install -y $(cat "toolbox/pkgs/centos-stream9.txt")

# Install packages via the Python package-mange (pip)
python3 -m pip install meson ninja pyelftools
