#!/usr/bin/env bash
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via dnf
zypper --non-interactive install -y $(cat "toolbox/pkgs/opensuse-leap-15.4.txt")
