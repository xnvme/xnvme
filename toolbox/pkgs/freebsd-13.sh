#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via pkg
pkg install -qy $(cat "toolbox/pkgs/freebsd-13.txt")
