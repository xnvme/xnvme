#!/usr/bin/env bash
# configure xNVMe and build meson subprojects(SPDK)
# --prefix=/usr to install directly into the system paths, avoiding the need
# to reconfigure the dynamic linker for /usr/local
meson setup builddir --prefix=/usr

# build xNVMe
meson compile -C builddir

# install xNVMe
meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
