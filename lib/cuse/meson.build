# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

# Not part of libxnvme: a static library linked only into the tools that serve
# the NVMe-driver ioctl() mimic (homi, qublk), so libxnvme exports none of it
xnvme_cuse_inc = include_directories('.')
xnvme_cuse_lib = static_library(
  'xnvme_cuse',
  files('xnvme_cuse.c'),
  include_directories: [conf_inc, xnvme_inc],
  link_with: xnvmelib,
  dependencies: [thread_dep],
  install: false,
)
