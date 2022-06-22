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

# The meson version available via yum is tool old < 0.54 and the Python version is tool old to
# support the next release of meson, so to fix this, Python3 is installed from source and meson
# installed via pip3 / PyPI
PY_VERSION=3.7.12
wget https://www.python.org/ftp/python/${PY_VERSION}/Python-${PY_VERSION}.tgz
tar xzf Python-${PY_VERSION}.tgz
cd Python-${PY_VERSION}

# PATCH: https://stackoverflow.com/questions/5937337/building-python-with-ssl-support-in-non-standard-location
cat << EOF >> ./Modules/Setup.dist
SSL=/usr/local/ssl
_ssl _ssl.c \
	-DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl \
	-L$(SSL)/lib -lssl -lcrypto

EOF

./configure --enable-optimizations --enable-shared
make altinstall -j $(nproc)

# Setup handling of python3 and pip3
rm -f /usr/local/bin/pip3 || true
rm -f /usr/local/bin/python3 || true
ln -s /usr/local/bin/pip3.7 /usr/local/bin/pip3
ln -s /usr/local/bin/python3.7 /usr/local/bin/python3
hash -d python3 pip3 || true

# Avoid error with "libpython3.7m.so.1.0: cannot open shared object file: No such file or directory"
ldconfig /usr/local/lib

# Install packages via the Python package-manager (pip)
python3 -m pip install meson ninja pyelftools
