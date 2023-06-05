#!/bin/sh
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
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git
cd libvfn
git checkout v2.0.2
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
cd ..

