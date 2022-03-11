xnvmefioe_inc = is_windows ? [
  fio_winposix_inc,
  conf_inc,
  xnvme_inc
] : [ xnvme_inc ]

#xnvmefioe = shared_library(
xnvmefioe = shared_module(
  meson.project_name() + '-fio-engine',
  ['xnvme_fioe.c'],
  include_directories: xnvmefioe_inc,
  c_args: [
    '-g',
    '-Wno-unused-parameter',
    '-D_GNU_SOURCE',
    '-include', 'config-host.h',
  ],
  dependencies: [ fio_dep ],
  link_with: xnvmelib_static,
  link_args: (is_windows or is_darwin) ? '' : '-lrt',
  install: true,
)

install_data('xnvme-compare.fio', install_dir : get_option('datadir') / meson.project_name())
install_data('xnvme-verify.fio', install_dir : get_option('datadir') / meson.project_name())
install_data('xnvme-zoned.fio', install_dir : get_option('datadir') / meson.project_name())

# pkg-config -- describing how to find the xNVMe fio io-engine and the associated fio-scripts
pconf.generate(
  libraries: [ xnvmefioe ],
  version: meson.project_version(),
  variables: [
    'datadir=' + get_option('prefix') / get_option('datadir') / meson.project_name()
  ],
  name: 'xNVMe-fioe',
  filebase: 'xnvme-fio-engine',
  description : 'xNVMe fio I/O engine'
)
