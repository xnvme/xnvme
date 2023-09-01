#!/bin/sh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (apk)
apk add \
 bash \
 bsd-compat-headers \
 build-base \
 clang15-extra-tools \
 coreutils \
 cunit \
 findutils \
 gawk \
 git \
 libaio-dev \
 liburing-dev \
 libuuid \
 linux-headers \
 make \
 meson \
 musl-dev \
 nasm \
 ncurses \
 numactl-dev \
 openssl-dev \
 patch \
 perl \
 pkgconf \
 py3-pip \
 python3 \
 python3-dev \
 util-linux-dev

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git
cd libvfn
git checkout v2.0.2
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
cd ..

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

