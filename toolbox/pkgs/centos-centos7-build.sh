#!/usr/bin/env bash
# Source in the newer gcc-toolchain before building
source /opt/rh/devtoolset-8/enable

# configure xNVMe and build dependencies (fio and SPDK, disabling liburing)
meson setup builddir -Dwith-liburing=false --prefix=/usr

# build xNVMe
meson compile -C builddir

# install xNVMe
meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
