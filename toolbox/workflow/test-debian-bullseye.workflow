---
doc: |
  Test xNVMe on Linux

steps:
- name: sysinfo
  uses: linux.sysinfo

- name: test
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests'
