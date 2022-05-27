#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (pkg)
pkg install -qy $(cat "toolbox/pkgs/freebsd-12.txt")

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools
