#!/bin/sh

# Install packages via brew
brew update
brew install $(cat "scripts/pkgs/macos-11.txt")

# Install packages via PyPI
pip3 install meson ninja
