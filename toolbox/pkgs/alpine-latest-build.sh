#!/usr/bin/env bash
# configure xNVMe and build dependencies (fio, libvfn, and SPDK)
meson setup builddir -Dwith-spdk=false

# build xNVMe
meson compile -C builddir

# install xNVMe
meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
