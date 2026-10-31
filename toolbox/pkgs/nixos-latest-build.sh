#!/usr/bin/env bash
# Build xNVMe as its Nix package from this tree, then install it into the
# user's Nix profile, which puts its tools on the PATH
nix-build toolbox/nix -o result
nix-env --install ./result
xnvme env library
