#!/usr/bin/env bash
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
 libarchive-dev \
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
# - Dependencies for building libvfn are met (system packages etc.)
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git toolbox/third-party/libvfn/repository

pushd toolbox/third-party/libvfn/repository
git checkout v3.0.0-rc1
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

