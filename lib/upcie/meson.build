# SPDX-FileCopyrightText: Simon A. F. Lund
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_upcie_source = files(
  'xnvme_be_upcie_admin.c',
  'xnvme_be_upcie_async.c',
  'xnvme_be_upcie.c',
  'xnvme_be_upcie_dev.c',
  'xnvme_be_upcie_mem.c',
  'xnvme_be_upcie_sync.c',
)
xnvmelib_source += xnvmelib_upcie_source

if get_option('be_upcie') and is_linux
conf_inc += [ include_directories('.') ]
endif
