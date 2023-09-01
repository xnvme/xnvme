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
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git
cd libvfn
git checkout v2.0.2
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
cd ..

# Clone, build and install liburing v2.2
git clone https://github.com/axboe/liburing.git
cd liburing
git checkout liburing-2.2
./configure --libdir=/usr/lib64 --libdevdir=/usr/lib64
make
make install
cd ..

