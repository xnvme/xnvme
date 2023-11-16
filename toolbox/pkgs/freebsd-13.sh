#!/bin/tcsh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (pkg)
pkg install -qy \
 autoconf \
 automake \
 bash \
 cunit \
 devel/py-pyelftools \
 e2fsprogs-libuuid \
 findutils \
 git \
 gmake \
 isa-l \
 libtool \
 meson \
 nasm \
 ncurses \
 openssl \
 patch \
 pkgconf \
 py39-pipx \
 python3 \
 wget

# Upgrade pip
python3 -m ensurepip --upgrade

# Installing meson via pip as the one available 'pkg install' currently that is 0.62.2, breaks with
# a "File name too long", 0.60 seems ok.
python3 -m pip install meson==0.60

