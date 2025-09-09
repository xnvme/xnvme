#!/bin/bash
set -eux

adduser --disabled-password --gecos "" ghr
usermod -aG sudo ghr
usermod -aG kvm ghr

# Install /etc files from provisioning stash in /tmp
cp etc/security/limits.d/90-ghr-memlock.conf /etc/security/limits.d/

# Allow the user to do passwordless sudo
echo "ghr ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ghr
