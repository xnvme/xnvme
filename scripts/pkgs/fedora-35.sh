#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via dnf
dnf install -y $(cat "scripts/pkgs/fedora-35.txt")
