#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (pacman)
pacman -Syyu --noconfirm
pacman -S --noconfirm \
 base-devel \
 bash \
 clang \
 cunit \
 findutils \
 git \
 libaio \
 libarchive \
 liburing \
 libutil-linux \
 make \
 meson \
 nasm \
 ncurses \
 numactl \
 openssl \
 patch \
 pkg-config \
 python-pip \
 python-pipx \
 python-pyelftools \
 python-setuptools \
 python3 \
 util-linux-libs

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
./configure --prefix=/usr --libdir=/usr/lib
make
make install
popd

