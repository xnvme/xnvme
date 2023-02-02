fs = import('fs')

examples_source = [
  'xnvme_dev.c',
  'xnvme_enum.c',
  'xnvme_hello.c',
  'xnvme_io_async.c',
  'xnvme_single_async.c',
  'xnvme_single_sync.c',
  'zoned_io_async.c',
  'zoned_io_sync.c',
]

xnvme_dep = get_variable('lib_' + get_option('default_library'))

foreach example_source : examples_source
  executable(
    fs.stem(example_source),
    example_source,
    include_directories: [conf_inc, xnvme_inc],
    dependencies: xnvme_dep,
    link_whole: conf_data.get('XNVME_COMPILE_STATIC') ? [xnvme_static_lib] : [],
    install: true,
  )
endforeach
