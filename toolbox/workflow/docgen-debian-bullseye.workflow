---
doc: |
  Initialize guest for documentation generation and then generate the documentation

  * Build and install xNVMe from source
  * Build and install xNVMe Python packages from source
  * Prepare custom fio in /opt/custom_fio along with external engines

steps:
- name: kill
  uses: qemu.guest_kill

- name: initialize
  uses: qemu.guest_bootimg

- name: start
  uses: qemu.guest_start_nvme

- name: sysinfo
  uses: linux.sysinfo

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

- name: gha_docgen
  uses: gha_docgen
  with:
    xnvme_source: '/tmp/xnvme_source'
