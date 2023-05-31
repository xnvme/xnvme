#!/bin/sh
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

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

# Clone, build and install liburing v2.2
git clone https://github.com/axboe/liburing.git
pushd liburing
git checkout liburing-2.2
./configure
make
make install
popd

