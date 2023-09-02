# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tools_source = [
  'lblk.c',
  'xdd.c',
  'xnvme.c',
  'xnvme_file.c',
  'zoned.c',
  'kvs.c',
]

foreach source : tools_source
  bin_name = fs.stem(source)
  bin = executable(
    bin_name,
    source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib_static,
    install: true,
  )
endforeach
