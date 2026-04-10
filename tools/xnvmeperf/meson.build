# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

if thread_dep.found()
  xnvmeperf_sources = files('xnvmeperf.c')
  xnvmeperf_deps = [thread_dep]

  xnvmeperf_cuda_args = []
  if conf_data.get('XNVME_BE_UPCIE_CUDA_ENABLED')
    xnvmeperf_sources += files('xnvmeperf_cuda.cu')
    xnvmeperf_deps += [cuda_dep]
    xnvmeperf_cuda_args = ['-arch=native']
  endif

  executable(
    'xnvmeperf',
    xnvmeperf_sources,
    include_directories: [conf_inc, xnvme_inc],
    link_args: link_args_hardening,
    link_with: xnvmelib,
    install_rpath: xnvmelib_rpath,
    dependencies: xnvmeperf_deps,
    cuda_args: xnvmeperf_cuda_args,
    install: true,
  )
else
  warning('No support for threads, skipping xnvmeperf')
endif
