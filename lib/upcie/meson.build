# SPDX-FileCopyrightText: Simon A. F. Lund
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_upcie_source = files(
  'xnvme_be_upcie_admin.c',
  'xnvme_be_upcie_async.c',
  'xnvme_be_upcie_cuda_gpu.c',
  'xnvme_be_upcie.c',
  'xnvme_be_upcie_cuda.c',
  'xnvme_be_upcie_dev.c',
  'xnvme_be_upcie_cplane_client.c',
  'xnvme_be_upcie_cplane_export.c',
  'xnvme_be_upcie_cplane_serve.c',
  'xnvme_be_upcie_cplane_serve_conn.c',
  'xnvme_be_upcie_cplane_serve_queues.c',
  'xnvme_be_upcie_cuda_dev.c',
  'xnvme_be_upcie_mem.c',
  'xnvme_be_upcie_cplane.c',
  'xnvme_be_upcie_cuda_mem.c',
  'xnvme_be_upcie_sync.c',
  'xnvme_be_upcie_vfio.c',
  'xnvme_be_upcie_hip.c',
  'xnvme_be_upcie_hip_dev.c',
  'xnvme_be_upcie_hip_mem.c',
)
xnvmelib_source += xnvmelib_upcie_source

if get_option('be_upcie') and is_linux
conf_inc += [ include_directories('.') ]
endif
