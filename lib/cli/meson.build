# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

# Not part of libxnvme: a static library linked into the tools and tests, so
# libxnvme exports none of it and the flags of the tools can change without
# changing the API.
xnvme_cli_inc = include_directories('.')
xnvme_cli_lib = static_library(
  'xnvme_cli',
  files('xnvme_cli.c'),
  include_directories: [conf_inc, xnvme_inc],
  link_with: xnvmelib,
  install: false,
)
