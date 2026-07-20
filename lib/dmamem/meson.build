# SPDX-FileCopyrightText: Simon A. F. Lund
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_dmamem_source = files(
  'xnvme_be_dmamem.c',
  'xnvme_be_dmamem_admin.c',
  'xnvme_be_dmamem_async.c',
  'xnvme_be_dmamem_dev.c',
  'xnvme_be_dmamem_mem.c',
  'xnvme_be_dmamem_sync.c',
)
xnvmelib_source += xnvmelib_dmamem_source

if get_option('be_dmamem') and is_linux
conf_inc += [ include_directories('.') ]
endif
