#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via apk
#apk update
apk add $(cat scripts/pkgs/alpine-3.11.3.txt)
