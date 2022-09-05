#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (pkg)
pkg install -qy $(cat "toolbox/pkgs/freebsd-13.txt")

# Upgrade pip
python3 -m ensurepip --upgrade

# Installing meson via pip as the one available 'pkg install' currently that is 0.62.2, breaks with
# a "File name too long", 0.60 seems ok.
python3 -m pip install meson==0.60
