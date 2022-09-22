#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (dnf)
dnf install -y $(cat "toolbox/pkgs/fedora-34.txt")

# Install liburing-2.2 via rawhide repos
dnf install -y fedora-repos-rawhide
dnf install -y --disablerepo=* --enablerepo=rawhide liburing liburing-devel --nogpgcheck
