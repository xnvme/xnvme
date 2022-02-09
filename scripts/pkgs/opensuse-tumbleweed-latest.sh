#!/usr/bin/env bash
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via dnf
zypper --non-interactive install -y $(cat "scripts/pkgs/opensuse-tumbleweed-latest.txt")
