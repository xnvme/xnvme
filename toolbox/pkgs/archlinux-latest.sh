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
git clone https://github.com/OpenMPDK/libvfn.git toolbox/third-party/libvfn/repository

pushd toolbox/third-party/libvfn/repository
git checkout v3.0.0-rc1
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

