#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (dnf)
dnf install -y \
 CUnit-devel \
 autoconf \
 bash \
 clang-tools-extra \
 diffutils \
 findutils \
 g++ \
 gcc \
 git \
 libaio-devel \
 libarchive-devel \
 libtool \
 libuuid-devel \
 make \
 meson \
 nasm \
 ncurses \
 numactl-devel \
 openssl-devel \
 patch \
 pipx \
 pkgconfig \
 procps \
 python3-devel \
 python3-pip \
 python3-pyelftools \
 zlib-devel

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

# Clone, build and install liburing v2.2
#
# Assumptions:
#
# - Dependencies for building liburing are met (system packages etc.)
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/axboe/liburing.git toolbox/third-party/liburing/repository

pushd toolbox/third-party/liburing/repository
git checkout liburing-2.2
./configure --libdir=/usr/lib64 --libdevdir=/usr/lib64
make
make install
popd

