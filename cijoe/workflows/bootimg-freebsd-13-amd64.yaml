---
doc: |
  This prepares a boot.img from an official FreeBSD virtual machine image (BASIC-CI)

  The basic-ci images is altered by the addition of:

  * git
  * vim
  * htop
  * python3 (with pip)
  * Kernel source

  And the removal of

  * /usr/lib/debug
    - Removing as an image less than 5GB is needed for storage on DigitalOcean Spaces

  Previously, we used the unoffical cloud-init images, however, these are now stale.
  This method applied here works for our needs, however, having cloud-init would be.

  This script assumes that the bootimg config has the basic ci image, it can be retrieve

  * wget https://download.freebsd.org/releases/CI-IMAGES/13.1-RELEASE/amd64/Latest/FreeBSD-13.1-RELEASE-amd64-BASIC-CI.raw.xz
  * xz --decompress FreeBSD-13.1-RELEASE-amd64-BASIC-CI.raw.xz
  * qemu-img convert FreeBSD-13.1-RELEASE-amd64-BASIC-CI.raw FreeBSD-13.1-RELEASE-amd64-BASIC-CI.qcow2 -O qcow2

  After creating, then

  * Shut down the guest
  * Convert the image (with compression)
    - qemu-img convert boot.img /tmp/freebsd-13.1-ksrc-amd64.qcow2 -O qcow2 -c
  * Upload it to DigitalOcean

steps:
- name: kill
  uses: qemu.guest_kill

- name: initialize
  uses: qemu.guest_bootimg

- name: start
  uses: qemu.guest_start_nvme

- name: check
  run: |
    hostname
    uname -a

- name: update_packages
  run: |
    env ASSUME_ALWAYS_YES=yes pkg update -f
    env ASSUME_ALWAYS_YES=yes pkg upgrade -fy
    env ASSUME_ALWAYS_YES=yes pkg install -y git vim htop python3
    python3 -m ensurepip

- name: kernel_source
  run: |
    fetch https://download.freebsd.org/ftp/releases/amd64/13.1-RELEASE/src.txz
    tar xzfp src.txz -C /
    rm src.txz || true
    pkg clean -ay
    rm -rf /usr/lib/debug/
