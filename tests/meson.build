# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tests_prefix = 'xnvme_tests_'

tests = {
  'async_intf.c': [
    ['count=1', ['init_term', '1GB', '--count', '1', '--qdepth', '64']],
    ['count=2', ['init_term', '1GB', '--count', '2', '--qdepth', '64']],
    ['count=4', ['init_term', '1GB', '--count', '4', '--qdepth', '64']],
    ['count=8', ['init_term', '1GB', '--count', '8', '--qdepth', '64']],
    ['count=16', ['init_term', '1GB', '--count', '16', '--qdepth', '64']],
    ['count=32', ['init_term', '1GB', '--count', '32', '--qdepth', '64']],
  ],
  'buf.c': [
    ['alloc', ['buf_alloc_free', '1GB', '--count', '31']],
    ['virt_alloc', ['buf_virt_alloc_free', '1GB', '--count', '31']],
  ],
  'cli.c': [
    ['optional', ['optional']],
  ],
  'compare.c': [
    ['no args', ['compare', '1GB']],
    ['slba=0 nlb=7', ['compare', '1GB', '--slba', '0', '--nlb', '7']],
  ],
  'copy.c': [
    ['no args', ['copy', '1GB']],
    ['slba=0 sdlba=8', ['copy', '1GB', '--slba', '0', '--nlb', '7', '--sdlba', '8']],
    ['slba=8 sdlba=0', ['copy', '1GB', '--slba', '8', '--nlb', '7', '--sdlba', '0']]],
  'delay_identification.c': [
    ['open', ['open', '1GB']]
  ],
  'enum.c': [
    ['open', ['open', '--count', '4']],
    ['multi', ['multi', '--count', '4']],
  ],
  'ioworker.c': [
    ['verify', ['verify', '1GB']],
    ['verify_sync', ['verify-sync', '1GB']],
    ['verify vec-cnt=4', ['verify', '1GB', '--vec-cnt', '4']],
    ['verify_sync vec-cnt=4', ['verify-sync', '1GB', '--vec-cnt', '4']],
    ['verify direct=1', ['verify', '1GB', '--direct', '1']],
    ['verify_sync direct=1', ['verify-sync', '1GB', '--direct', '1']],
    ['verify vec-cnt=4 direct=1', ['verify', '1GB', '--vec-cnt', '4', '--direct', '1']],
    ['verify_sync vec-cnt=4 direct=1', ['verify-sync', '1GB', '--vec-cnt', '4', '--direct', '1']],
  ],
  'kvs.c': [],
  'lblk.c': [
    ['io', ['io', '1GB']],
    ['write_zeroes', ['write_zeroes', '1GB']],
  ],
  'scc.c': [
    ['idfy', ['idfy', '1GB']],
    ['support', ['support', '1GB']],
    ['scopy', ['scopy', '1GB']],
    ['scopy clear', ['scopy', '1GB', '--clear']],
    ['scopy-msrc', ['scopy-msrc', '1GB']],
    ['scopy-msrc clear', ['scopy-msrc', '1GB', '--clear']],
  ],
  'xnvme_cli.c': [],
  'xnvme_file.c': [],
  'znd_append.c': [],
  'znd_explicit_open.c': [],
  'znd_state.c': [],
  'znd_zrwa.c': [],
}

if not is_windows
  tests += {'map.c': []}
endif

foreach test_source, tests_args : tests
  e = executable(
    tests_prefix + fs.stem(test_source),
    test_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib_static,
    link_args: link_args_hardening,
    install: true,
  )
  foreach test_args : tests_args
    test_name = test_args[0]
    test('tests - '+fs.stem(test_source)+', '+test_name, e, args : test_args[1])
  endforeach
endforeach