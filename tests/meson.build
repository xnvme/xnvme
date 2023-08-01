# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tests_prefix = 'xnvme_tests_'
tests_source = [
  'async_intf.c',
  'buf.c',
  'cli.c',
  'enum.c',
  'lblk.c',
  'scc.c',
  'ioworker.c',
  'xnvme_file.c',
  'xnvme_cli.c',
  'znd_append.c',
  'znd_explicit_open.c',
  'znd_state.c',
  'znd_zrwa.c',
  'kvs.c',
]

if not is_windows
  tests_source += ['map.c']
endif

foreach test_source : tests_source
  executable(
    tests_prefix + fs.stem(test_source),
    test_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib_static,
    install: true,
  )
endforeach
