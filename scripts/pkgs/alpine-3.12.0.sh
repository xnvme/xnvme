#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via apk
apk add $(cat "scripts/pkgs/alpine-3.12.0.txt")

# Install packages via PyPI
pip3 install meson ninja
