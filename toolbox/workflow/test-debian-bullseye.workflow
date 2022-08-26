---
doc: |
  Test xNVMe on Linux

steps:
- name: sysinfo
  uses: linux.sysinfo

- name: hugetlbfs
  uses: gha_prepare_hugetlbfs

- name: test_ramdisk
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests -k "ramdisk"'

- name: test_os
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests -k "(not ramdisk) and (not pcie) and (not fabrics)"'

- name: test_pcie
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests -k "pcie"'

- name: test_fabrics
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests -k "fabrics and not ioworker"'
