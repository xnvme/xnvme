# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

examples_source = [
  'xnvme_dev.c',
  'xnvme_enum.c',
  'xnvme_hello.c',
  'xnvme_io_async.c',
  'xnvme_single_async.c',
  'xnvme_single_async_efd.c',
  'xnvme_single_sync.c',
  'zoned_io_async.c',
  'zoned_io_sync.c',
]

# Installed as 'xnvme-example-<name>', run as 'xnvme example <name>'
foreach example_source : examples_source
  executable(
    'xnvme-example-' + fs.stem(example_source).replace('xnvme_', '').replace('_', '-'),
    example_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib,
    link_args: link_args_hardening,
    install_rpath: xnvmelib_rpath,
    install: true,
    install_dir: xnvme_libexecdir,
  )
endforeach

if conf_data.get('XNVME_BE_UPCIE_HIP_ENABLED')
  executable(
    'xnvme-example-hip-mem-map',
    'xnvme_hip_mem_map.c',
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib,
    dependencies: [hip_dep],
    link_args: link_args_hardening,
    install_rpath: xnvmelib_rpath,
    install: true,
    install_dir: xnvme_libexecdir,
  )
endif

if conf_data.get('XNVME_BE_UPCIE_CUDA_ENABLED')
  foreach example: ['xnvme_cuda', 'xnvme_cuda_mem_map']
    executable(
      'xnvme-example-' + example.replace('xnvme_', '').replace('_', '-'),
      example + '.cu',
      include_directories: [conf_inc, xnvme_inc],
      link_with: xnvmelib,
      dependencies: [cuda_dep],
      cuda_args: cuda_arch_args,
      link_args: link_args_hardening,
      install_rpath: xnvmelib_rpath,
      install: true,
      install_dir: xnvme_libexecdir,
    )
  endforeach
endif
