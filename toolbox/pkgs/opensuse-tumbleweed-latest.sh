#!/usr/bin/env bash
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via zypper
zypper --non-interactive install -y --allow-downgrade $(cat "toolbox/pkgs/opensuse-tumbleweed-latest.txt")
