#!/usr/bin/env sh
find patches -type f -name '0*.patch' -print0 | sort -z | xargs -t -0 -n 1 patch -p1 --forward -i || true
cd dpdk
patch -p1 --forward < ../patches/spdk-dpdk-no-plugins.patch || true
patch -p1 --forward < ../patches/spdk-dpdk-no-libarchive.patch || true
