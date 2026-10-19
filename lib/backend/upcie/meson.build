# SPDX-FileCopyrightText: Simon A. F. Lund
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_upcie_source = files(
  'xnvme_be_upcie_admin.c',
  'xnvme_be_upcie_async.c',
  'xnvme_be_upcie_configs.c',
  'xnvme_be_upcie_cuda_configs.c',
  'xnvme_be_upcie_cuda_gpu.c',
  'xnvme_be_upcie_dev.c',
  'xnvme_be_upcie_dev_cuda.c',
  'xnvme_be_upcie_dev_hip.c',
  'xnvme_be_upcie_hip_configs.c',
  'xnvme_be_upcie_mem.c',
  'xnvme_be_upcie_mem_cuda.c',
  'xnvme_be_upcie_mem_hip.c',
  'xnvme_be_upcie_mproc.c',
  'xnvme_be_upcie_sync.c',
  'xnvme_be_upcie_vfio.c',
)
xnvmelib_source += xnvmelib_upcie_source

if be_upcie
xnvmelib_inc += include_directories('.')
endif
