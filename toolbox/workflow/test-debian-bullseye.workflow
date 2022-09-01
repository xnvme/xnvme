---
doc: |
  Test xNVMe

steps:
- name: sysinfo
  uses: linux.sysinfo

- name: test
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests'
