#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Enable CRB repo for development packages
dnf install -y dnf-plugins-core
dnf config-manager --set-enabled ol10_codeready_builder

# Enable EPEL for additional packages
dnf install -y oracle-epel-release-el10

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
 liburing \
 liburing-devel \
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
 unzip \
 wget \
 zlib-devel

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

