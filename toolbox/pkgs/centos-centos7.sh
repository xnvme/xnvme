#!/bin/sh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Only CentOS 7 uses yum, the other distros have dnf
# Install Developer Tools
yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
yum-config-manager --enable epel
yum install -y centos-release-scl
yum-config-manager --enable rhel-server-rhscl-7-rpms
yum install -y devtoolset-8

# Install packages via the system package-manager (yum)
yum install -y \
 CUnit-devel \
 autoconf \
 bash \
 clang-format \
 findutils \
 git \
 glibc-static \
 libaio-devel \
 libffi-devel \
 libtool \
 libuuid-devel \
 make \
 ncurses-devel \
 numactl-devel \
 openssl-devel \
 openssl11-devel \
 patch \
 pkgconfig \
 unzip \
 wget \
 zlib-devel

# Install nasm v2.15 from source
git clone https://github.com/netwide-assembler/nasm.git
cd nasm
git checkout nasm-2.15
./autogen.sh
./configure --prefix=/usr
make -j $(nproc)
make install
cd ..

# Install Python v3.7.12 from source
wget https://www.python.org/ftp/python/3.7.12/Python-3.7.12.tgz
tar xzf Python-3.7.12.tgz
cd Python-3.7.12
./configure --enable-optimizations --enable-shared
make altinstall -j $(nproc)
cd ..

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
cd libvfn
git checkout v1.0.0

# Source in the newer gcc-toolchain before building
source /opt/rh/devtoolset-8/enable

meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
cd ..

