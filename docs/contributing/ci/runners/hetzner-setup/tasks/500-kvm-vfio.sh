#!/bin/bash
set -eux

# Load vfio-pci on boot and 
cp etc/modules-load.d/vfio-pci.conf /etc/modules-load.d/

# Allow kvm-group access to vfio
cp etc/udev/rules.d/99-vfio-kvm.rules /etc/udev/rules.d/
udevadm control --reload-rules
udevadm trigger --subsystem-match=vfio

# Run the devbind script after modules-load
cp etc/systemd/system/post-vfio.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now post-vfio.service
