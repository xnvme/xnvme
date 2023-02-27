#!/bin/sh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# This is done to avoid appleframework deprecation warnings
export MACOSX_DEPLOYMENT_TARGET=11.0

# Install packages via brew, assuming that brew is: installed, updated, and upgraded
clang-format --version && echo "Installed" || brew install clang-format
git --version && echo "Installed" || brew install git
meson --version && echo "Installed" || brew install meson
pkg-config --version && echo "Installed" || brew install pkg-config
python3 --version && echo "Installed" || brew install python3

