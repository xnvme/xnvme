# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

if thread_dep.found()
  executable(
    'xnvmeperf',
    files('xnvmeperf.c'),
    include_directories: [conf_inc, xnvme_inc],
    link_args: link_args_hardening,
    link_with: xnvmelib,
    install_rpath: xnvmelib_rpath,
    dependencies: [thread_dep],
    install: true,
  )
else
  warning('No support for threads, skipping xnvmeperf')
endif
