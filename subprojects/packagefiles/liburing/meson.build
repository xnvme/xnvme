project(
  'liburing',
  'c',
  version: '2.2'
)
fs = import('fs')

get_option('build_subprojects')

if get_option('build_subprojects') and not fs.exists('config-host.h')
  message('Patching ..')
  run_command('./liburing_patches.sh', capture: true, check: true)
  message('Configuring ..')
  run_command('./liburing_configure.sh', capture: true, check: true)
endif
if get_option('build_subprojects') and not fs.exists('src' / 'liburing.a')
  run_command('./liburing_build.sh', capture: true, check: true)
endif

liburing_lib = fs.exists('src' / 'liburing.a') ? meson.get_compiler('c').find_library(
  'uring',
  dirs: [
    meson.current_source_dir() / 'src',
    meson.current_source_dir() / 'src' / 'include'
  ],
  static: true
) : []

liburing_inc = include_directories( 'src' / 'include')
liburing_dep = declare_dependency(
  dependencies : liburing_lib,
  include_directories: liburing_inc,
  variables: {
    'lib_paths': meson.current_source_dir() / 'src' / 'liburing.a',
    'inc_paths': meson.current_source_dir() / 'src' / 'include'

  }
)

meson.override_dependency('uring', liburing_dep)
