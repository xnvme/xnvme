#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install Developer Tools
yum install -y centos-release-scl
yum-config-manager --enable rhel-server-rhscl-7-rpms
yum install -y devtoolset-8

# Install packages via the system package-manager (yum)
yum install -y $(cat "toolbox/pkgs/centos-centos7.txt")

# Install nasm from source
git clone https://github.com/netwide-assembler/nasm.git
cd nasm
git checkout nasm-2.15
./autogen.sh
./configure --prefix=/usr
make -j $(nproc)
make install
cd ..

# The meson version available via yum is tool old < 0.54 and the Python version is tool old to
# support the next release of meson, so to fix this, Python3 is installed from source and meson
# installed via the Python package-manager pip
wget https://www.python.org/ftp/python/3.7.12/Python-3.7.12.tgz
tar xzf Python-3.7.12.tgz
cd Python-3.7.12
./configure --enable-optimizations --enable-shared
make altinstall -j $(nproc)

# Setup handling of python3
ln -s /usr/local/bin/python3.7 /usr/local/bin/python3
hash -d python3 || true

# Avoid error with "libpython3.7m.so.1.0: cannot open shared object file: No such file or directory"
ldconfig /usr/local/lib

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools
