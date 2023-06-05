#!/usr/bin/env bash

# This is done to avoid appleframework deprecation warnings
export MACOSX_DEPLOYMENT_TARGET=11.0

# configure xNVMe
meson setup builddir

# build xNVMe
meson compile -C builddir

# install xNVMe
sudo meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
