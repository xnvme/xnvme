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

foreach example_source : examples_source
  executable(
    fs.stem(example_source),
    example_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib,
    link_args: link_args_hardening,
    install_rpath: xnvmelib_libdir,
    install: true,
  )
endforeach
