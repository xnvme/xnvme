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

# This only works because on a AX42 instance there are two NVMe SSDs and
# Linux configures them with BDFs 0000:01:0.0 and 0000:02:0.0, also,the NVMe
# controller behind that BDF has a single NVM namespace.
NVME_NAME=$(basename /sys/bus/pci/devices/0000\:01\:00.0/nvme/nvme*/nvme*)

# Setup the install configuration
cp "$SETUP_DIR/install.conf.tmpl" "$SETUP_DIR/install.conf"

sed -i "s/__NVME_NAME__/${NVME_NAME}/g" "$SETUP_DIR/install.conf"
sed -i "s/__HOSTNUMBER__/${NUMBER}/g" "$SETUP_DIR/install.conf"

# Run installimage with patched config
installimage -a -c "$SETUP_DIR/install.conf"

# Drop scripts into /root
mount /dev/${NVME_NAME}p4 "$TARGET"
cp -ar "$SETUP_DIR" "$TARGET/root/."
umount "$TARGET"
