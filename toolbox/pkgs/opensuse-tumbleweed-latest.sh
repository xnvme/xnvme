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

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

