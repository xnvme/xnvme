# SPDX-FileCopyrightText: Simon A. F. Lund
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_upcie_source = files(
  'xnvme_be_upcie_admin.c',
  'xnvme_be_upcie_async.c',
  'xnvme_be_upcie_cuda_async.c',
  'xnvme_be_upcie_cuda_gpu.c',
  'xnvme_be_upcie.c',
  'xnvme_be_upcie_cuda.c',
  'xnvme_be_upcie_dev.c',
  'xnvme_be_upcie_cuda_dev.c',
  'xnvme_be_upcie_mem.c',
  'xnvme_be_upcie_mproc.c',
  'xnvme_be_upcie_cuda_mem.c',
  'xnvme_be_upcie_sync.c',
  'xnvme_be_upcie_vfio.c',
  'xnvme_be_upcie_hip.c',
  'xnvme_be_upcie_hip_async.c',
  'xnvme_be_upcie_hip_cqmirror.c',
  'xnvme_be_upcie_hip_dev.c',
  'xnvme_be_upcie_hip_mem.c',
)
xnvmelib_source += xnvmelib_upcie_source

if conf_data.get('XNVME_BE_UPCIE_CUDA_ENABLED')
  xnvmelib_source += files('xnvme_be_upcie_cuda_cqmirror.cu')
endif

# The library is a gcc build against the HIP runtime; the one kernel it has is
# compiled to a code object by hipcc on its own and embedded, to be loaded
# through the module API.
if conf_data.get('XNVME_BE_UPCIE_HIP_ENABLED')
  hipcc = find_program('hipcc', dirs: ['/opt/rocm/bin'])
  hip_cqmirror_co = custom_target(
    'xnvme_be_upcie_hip_cqmirror.co',
    input: 'xnvme_be_upcie_hip_cqmirror.hip',
    output: 'xnvme_be_upcie_hip_cqmirror.co',
    depend_files: 'xnvme_be_upcie_hip_cqmirror.h',
    command: [
      hipcc, '--genco', '--offload-arch=native', '-O2',
      '-I', meson.current_source_dir(),
      '@INPUT@', '-o', '@OUTPUT@',
    ],
  )
  xnvmelib_source += custom_target(
    'xnvme_be_upcie_hip_cqmirror_co.c',
    input: hip_cqmirror_co,
    output: 'xnvme_be_upcie_hip_cqmirror_co.c',
    command: [
      find_program('python3'), meson.project_source_root() / 'toolbox' / 'meson_embed.py',
      '@INPUT@', '@OUTPUT@', 'xnvme_be_upcie_hip_cqmirror_co',
    ],
  )
endif

if get_option('be_upcie') and is_linux
conf_inc += [ include_directories('.') ]
endif
