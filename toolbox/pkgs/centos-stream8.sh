#!/bin/sh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# This repos has CUnit-devel
dnf install -y 'dnf-command(config-manager)' || true
dnf config-manager --set-enabled powertools || true

# Install packages via the system package-manager (dnf)
dnf install -y \
 CUnit-devel \
 autoconf \
 bash \
 diffutils \
 findutils \
 gcc \
 gcc-c++ \
 git \
 libaio-devel \
 libffi-devel \
 libtool \
 libuuid-devel \
 make \
 nasm \
 ncurses \
 numactl-devel \
 openssl-devel \
 patch \
 pkgconfig \
 procps \
 unzip \
 wget \
 zlib-devel

# Clone, build and install liburing v2.2
git clone https://github.com/axboe/liburing.git
pushd liburing
git checkout liburing-2.2
./configure --libdir=/usr/lib64 --libdevdir=/usr/lib64
make
make install
popd

# Install Python v3.7.12 from source
wget https://www.python.org/ftp/python/3.7.12/Python-3.7.12.tgz
tar xzf Python-3.7.12.tgz
pushd Python-3.7.12
./configure --enable-optimizations --enable-shared
make altinstall -j $(nproc)
popd

# Setup handling of python3
ln -s /usr/local/bin/python3.7 /usr/local/bin/python3
hash -d python3 || true

# Avoid error with "libpython3.7m.so.1.0: cannot open shared object file: No such file or directory"
ldconfig /usr/local/lib

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 meson \
 ninja \
 pipx \
 pyelftools

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

