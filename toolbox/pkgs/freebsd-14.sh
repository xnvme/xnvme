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
 devel/py-pipx \
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
 python3 \
 wget

