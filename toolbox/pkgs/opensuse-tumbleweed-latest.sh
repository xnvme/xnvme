#!/bin/sh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via the system package-manager (zypper)
zypper --non-interactive install -y --allow-downgrade \
 autoconf \
 bash \
 clang-tools \
 cunit-devel \
 findutils \
 gcc \
 gcc-c++ \
 git \
 gzip \
 libaio-devel \
 libnuma-devel \
 libopenssl-devel \
 libtool \
 liburing-devel \
 libuuid-devel \
 make \
 meson \
 nasm \
 ncurses \
 patch \
 pkg-config \
 python3 \
 python3-devel \
 python3-pip \
 python3-pyelftools \
 tar

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git
pushd libvfn
git checkout v1.0.0
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

