#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install Developer Tools
yum install -y centos-release-scl
yum-config-manager --enable rhel-server-rhscl-7-rpms
yum install -y devtoolset-8

# Source them in for usage before building xNVMe
source /opt/rh/devtoolset-8/enable

# Install packages via yum
yum install -y $(cat scripts/pkgs/centos-7.txt)

# Install CMake using installer from GitHUB
wget https://github.com/Kitware/CMake/releases/download/v3.16.5/cmake-3.16.5-Linux-x86_64.sh -O cmake.sh
chmod +x cmake.sh
./cmake.sh --skip-license --prefix=/usr/
