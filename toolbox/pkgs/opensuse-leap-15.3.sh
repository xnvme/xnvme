#!/usr/bin/env bash

# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via the system package-manager (zypper)
zypper --non-interactive install -y $(cat "toolbox/pkgs/opensuse-leap-15.3.txt")

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools

# Clone, build and install liburing
git clone https://github.com/axboe/liburing.git
cd liburing
git checkout liburing-2.2
./configure --libdir=/usr/lib64 --libdevdir=/usr/lib64
make
make install
