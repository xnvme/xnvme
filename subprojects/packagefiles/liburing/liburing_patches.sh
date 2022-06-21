#!/usr/bin/env sh
for patchfile in $(find patches -type f -name '0*.patch' | sort); do
  patch -p1 --forward -i $patchfile
done
