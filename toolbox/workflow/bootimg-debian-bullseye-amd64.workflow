---
doc: |
  Create a Debian Bullseye boot.img using a cloud-init image and Linux kernel .deb packages

  After creating, then
  * Shut down the guest
  * Convert the image (with compression)
    - qemu-img convert boot.img /tmp/debian-bullseye-amd64.qcow2 -O qcow2 -c
  * Upload it to DigitalOcean

steps:
- name: guest_kill
  uses: qemu.guest_kill

- name: guest_cloudinit
  uses: qemu.guest_cloudinit

- name: guest_firstboot
  uses: qemu.guest_start_nvme

- name: guest_check
  run: |
    hostname
    uname -a

- name: guest_update
  run: |
    apt-get -qy -o "DPkg::Lock::Timeout=180" -o "Dpkg::Options::=--force-confdef" -o "Dpkg::Options::=--force-confold" upgrade
    apt-get -qy autoclean
    apt-get -qy install aptitude
    aptitude -q -y -f install git

- name: linux_install_kdebs
  uses: linux.transfer_and_install_kdebs
  with:
    local_kdebs_dir: "kdebs"

- name: guest_grub_default
  run: |
    update-grub
    grub-set-default 1
    update-grub
    sync

- name: guest_info
  uses: linux.sysinfo
