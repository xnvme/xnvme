#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# This repos has CUnit-devel
dnf config-manager --set-enabled powertools

# Install packages via the system package-manager (dnf)
dnf install -y $(cat "toolbox/pkgs/centos-stream8.txt")

pip3 install meson ninja pyelftools
