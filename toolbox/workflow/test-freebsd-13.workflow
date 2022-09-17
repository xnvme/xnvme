---
doc: |
  Test xNVMe on FreeBSD

steps:
- name: sysinfo
  uses: freebsd_sysinfo

- name: kldconfig
  uses: kldconfig
  with:
    xnvme_source: '/tmp/xnvme_source'

- name: test
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests'
