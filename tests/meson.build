fs = import('fs')

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
  'xnvmec.c',
  'znd_append.c',
  'znd_explicit_open.c',
  'znd_state.c',
  'znd_zrwa.c',
]

xnvme_dep = get_variable('lib_' + get_option('default_library'))

foreach test_source : tests_source
  executable(
    tests_prefix + fs.stem(test_source),
    test_source,
    include_directories: [conf_inc, xnvme_inc],
    dependencies: xnvme_dep,
    link_whole: conf_data.get('XNVME_COMPILE_STATIC') ? [xnvme_static_lib] : [],
    install: true,
  )
endforeach
