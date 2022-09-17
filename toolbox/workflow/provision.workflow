---
doc: |
  Provision a guest using bootimg and xNVMe source

  The guest is created based of the "bootimg" in the config.

  On "local" the folder "/tmp/artifacts", must contain

  * xnvme-core.tar.gz
  * xnvme-cy-bindings.tar.gz
  * xnvme-cy-header.tar.gz
  * xnvme.tar.gz

  These will be transferred to the guest, and unpacked in "/tmp/xnvme_source", then built and
  installed.

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

- name: gha_prepare_source
  uses: gha_prepare_source
  with:
    artifacts: '/tmp/artifacts'
    xnvme_source: '/tmp/xnvme_source'

- name: build_prep
  uses: xnvme.build_prep
  with:
    xnvme_source: '/tmp/xnvme_source'

- name: build
  uses: xnvme.build
  with:
    xnvme_source: '/tmp/xnvme_source'

- name: install
  uses: xnvme.install
  with:
    xnvme_source: '/tmp/xnvme_source'

- name: gha_prepare_aux
  uses: gha_prepare_aux
  with:
    xnvme_source: '/tmp/xnvme_source'

- name: gha_prepare_python
  uses: gha_prepare_python
  with:
    xnvme_source: '/tmp/xnvme_source'
