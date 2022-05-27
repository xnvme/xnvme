#!/bin/sh
# Query the linker version
#ld --version || true

# Query the (g)libc version
ldd || true

# Install packages via the system package-manager (apk)
apk add $(cat "toolbox/pkgs/alpine-latest.txt")
