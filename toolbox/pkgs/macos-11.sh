#!/bin/sh

# Install packages via brew, assuming that brew is: installed, updated, and upgraded
clang-format --version && echo "Installed" || brew install clang-format
git --version && echo "Installed" || brew install git
python3 --version && echo "Installed" || brew install python3
meson --version && echo "Installed" || brew install meson
pkg-config --version && echo "Installed" || brew install pkg-config
