#!/bin/bash
shopt -s expand_aliases
set -eux

alias installimage='/root/.oldroot/nfs/install/installimage'

if [ $# -ne 1 ]; then
    echo "Usage: $0 <number>"
    exit 1
fi
NUMBER="$1"

SETUP_DIR="$PWD"
TARGET="/mnt"

# Wipe old superblocks / bootloaders
lsblk
wipefs -a /dev/sd? || true
wipefs -a /dev/nvme?n? || true
lsblk

# Patch install.conf with hostname number
sed "s/__HOSTNUMBER__/${NUMBER}/g" "$SETUP_DIR/install.conf.tmpl" > "$SETUP_DIR/install.conf"

# Run installimage with patched config
installimage -a -c "$SETUP_DIR/install.conf"

# Drop scripts into /root
mount /dev/nvme0n1p4 "$TARGET"
cp -ar "$SETUP_DIR" "$TARGET/root/."
umount "$TARGET"
