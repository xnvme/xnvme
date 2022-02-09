#!/usr/bin/env bash
export LDFLAGS="-ltinfo -lncurses"

# configure xNVMe and build dependencies (fio, liburing, and SPDK)
meson setup builddir

# build xNVMe
meson compile -C builddir

# install xNVMe
meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
