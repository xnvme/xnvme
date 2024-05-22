#!/bin/bash

# List of versions
versions=("next" "v0.7.4")

rm -r $HOME/git/xnvme.github.io/en
mkdir -p $HOME/git/xnvme.github.io/en

# Loop through each version
for VER in "${versions[@]}"; do
  sed -i "s/version = .*/version = \"$VER\"/" conf.py

  make sphinx

  cp -r builddir/html $HOME/git/xnvme.github.io/en/$VER
done

# Copy the build directory to the destination, when testing this locally then
# change the --url to "localhost"
./dest.py \
  --docs builddir/html \
  --site $HOME/git/xnvme.github.io \
  --ref refs/heads/docs \
  --url "https://xnvme.io"
