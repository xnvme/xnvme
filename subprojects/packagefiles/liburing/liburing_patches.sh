#!/usr/bin/env sh
for patch in $(find patches -type f -name '0*.patch' | sort); do
  patch -p1 --forward -i $patch
done
