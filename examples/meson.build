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

foreach example_source : examples_source
  executable(
    fs.stem(example_source),
    example_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib_static,
    install: true,
  )
endforeach
