fs = import('fs')

tools_source = [
  'lblk.c',
  'nvmec.c',
  'xdd.c',
  'xnvme.c',
  'xnvme_file.c',
  'zoned.c',
]

xnvme_dep = get_variable('lib_' + get_option('default_library'))

foreach source : tools_source
  bin_name = fs.stem(source)
  bin = executable(
    bin_name,
    source,
    include_directories: [conf_inc, xnvme_inc],
    dependencies: xnvme_dep,
    link_whole: conf_data.get('XNVME_COMPILE_STATIC') ? [xnvme_static_lib] : [],
    install: true,
  )
endforeach
