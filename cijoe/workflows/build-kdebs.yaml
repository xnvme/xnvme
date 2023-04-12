---
doc: |
  Build a Linux Kernel as Debian package

  Assumptions

  * Running the same deb-distro as the kdebs will be install on
  * Kernel-config will be based on the current system

  So basically, this is useful for getting a more recent kernel when running Debian

steps:
- name: sysinfo
  uses: linux.sysinfo

- name: build
  uses: linux.build_deb
