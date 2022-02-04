#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via pkg
pkg install -qy $(cat "scripts/pkgs/freebsd-13.txt")

# Make sure you have the kernel source, the headers are needed
#wget -O /usr/src/src.txz https://download.freebsd.org/ftp/releases/amd64/13.0-RELEASE/src.txz
#tar -C /usr/src/ -xf /usr/src/src.txz
#git clone --branch releng/13.0 https://git.FreeBSD.org/src.git --single-branch --depth=1 /usr/src
#svnlite checkout https://svn.freebsd.org/base/releng/13.0/sys/ /usr/src/sys
