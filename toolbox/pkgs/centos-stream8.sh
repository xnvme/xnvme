#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# This repos has CUnit-devel
dnf config-manager --set-enabled powertools

# Install packages via the system package-manager (dnf)
dnf install -y $(cat "toolbox/pkgs/centos-stream8.txt")

# Install packages via the Python package-mange (pip)
python3 -m pip install meson ninja pyelftools

# Clone, build and install liburing
git clone https://github.com/axboe/liburing.git
cd liburing
git checkout liburing-2.2
./configure --libdir=/usr/lib64 --libdevdir=/usr/lib64
make
make install
