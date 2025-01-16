#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# This repo has CUnit-devel + meson
dnf install -y 'dnf-command(config-manager)'
dnf config-manager --set-enabled crb

# Install packages via the system package-manager (dnf)
dnf install -y \
 CUnit-devel \
 autoconf \
 bash \
 clang-tools-extra \
 diffutils \
 findutils \
 gcc \
 gcc-c++ \
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
 pkgconfig \
 procps \
 python3-devel \
 python3-pip \
 python3-pyelftools \
 unzip \
 wget \
 zlib-devel

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

#
# Clone, build and install libisal
#
# Assumptions:
#
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/intel/isa-l.git toolbox/third-party/libisal/repository

pushd toolbox/third-party/libisal/repository
git checkout v2.30.0
./autogen.sh
./configure --prefix=/usr --libdir=/usr/lib64
make
make install
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

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - Dependencies for building libvfn are met (system packages etc.)
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/SamsungDS/libvfn.git toolbox/third-party/libvfn/repository

pushd toolbox/third-party/libvfn/repository
git checkout v5.1.0
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --buildtype=release --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

