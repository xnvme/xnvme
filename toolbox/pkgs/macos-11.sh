#!/bin/sh

# Install packages via brew
brew update
brew upgrade
clang-format --version && echo "Installed" || brew install clang-format
git --version && echo "Installed" || brew install git
python3 --version && echo "Installed" || brew install python3
meson --version && echo "Installed" || brew install meson
