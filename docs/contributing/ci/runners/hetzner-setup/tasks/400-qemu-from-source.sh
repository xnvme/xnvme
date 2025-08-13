#!/bin/bash
set -eux

mkdir -p $HOME/git
git clone --recursive https://github.com/SamsungDS/qemu.git $HOME/git/qemu
cd $HOME/git/qemu
git checkout for-xnvme
mkdir -p /opt/qemu
mkdir build
cd build
../configure \
  --prefix=/opt/qemu \
  --python=/usr/bin/python3 \
  --audio-drv-list="" \
  --disable-docs \
  --disable-debug-info \
  --disable-opengl \
  --disable-virglrenderer \
  --disable-vte \
  --disable-gtk \
  --disable-sdl \
  --disable-spice \
  --disable-vnc \
  --disable-curses \
  --disable-xen \
  --disable-smartcard \
  --disable-libnfs \
  --disable-libusb \
  --disable-glusterfs \
  --disable-tools \
  --disable-werror \
  --target-list="x86_64-softmmu"

make -j $(nproc)
make install