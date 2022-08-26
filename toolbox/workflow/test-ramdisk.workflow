---
doc: |
  Test xNVMe using the ramdisk backend

  Configuration
  =============

  * The tests utilize fio and thus the configuration must point to where custom fio is located
  * The test **should** otherwise run without any system dependencies as they only utilize the
    "ramdisk" backend

steps:
- name: test
  uses: core.testrunner
  with:
    args: '--pyargs cijoe.xnvme.tests -k "ramdisk and not hugepage"'
