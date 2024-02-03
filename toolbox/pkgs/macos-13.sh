#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# This is done to avoid appleframework deprecation warnings
export MACOSX_DEPLOYMENT_TARGET=11.0
export HOMEBREW_NO_AUTO_UPDATE=1

# Install packages via brew, assuming that brew is: installed, updated, and upgraded
clang-format --version && echo "Installed" || brew install clang-format --overwrite || echo "Failed installing"
git --version && echo "Installed" || brew install git --overwrite || echo "Failed installing"
isa-l --version && echo "Installed" || brew install isa-l --overwrite || echo "Failed installing"
if make --version | grep i386-apple; then
  brew install make
fi

meson --version && echo "Installed" || brew install meson --overwrite || echo "Failed installing"
pkg-config --version && echo "Installed" || brew install pkg-config --overwrite || echo "Failed installing"
python3 --version && echo "Installed" || brew install python3 --overwrite || echo "Failed installing"

# Install packages via the Python package-manager (pip)
python3 -m pip install --upgrade pip
python3 -m pip install \
 pipx

