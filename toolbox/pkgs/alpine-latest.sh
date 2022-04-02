#!/bin/sh
# Query the linker version
#ld --version || true

# Query the (g)libc version
ldd || true

# Install packages via apk
apk add $(cat "toolbox/pkgs/alpine-latest.txt")

# Install packages via PyPI
pip3 install pyelftools
