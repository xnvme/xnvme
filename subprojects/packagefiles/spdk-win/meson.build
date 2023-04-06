project(
  'spdk-win',
  'c',
  version: '22.05',
)
fs = import('fs')

if get_option('build_subprojects') and not fs.exists('build' / 'lib' / 'libspdk_nvme.a')
  message('Patching ..')
  run_command('spdk_win_patches.sh', capture: true, check: true)
  message('Configuring and building..')
  run_command('spdk_win_build.bat', capture: true, check: true)
endif

wpdk_libnames = get_option('build_subprojects') ? [
  'aio',
  'crypto',
  'numa',
  'pthread',
  'librt',
  'uuid',
  'wpdk'
] : []

spdk_libnames = get_option('build_subprojects') ? [
  'spdk_nvme',
  'spdk_env_dpdk',
  'spdk_sock',
  'spdk_sock_posix',
  'spdk_rpc',
  'spdk_trace',
  'spdk_jsonrpc',
  'spdk_json',
  'spdk_util',
  'spdk_log'
] : []

dpdk_libnames = get_option('build_subprojects') ? [
  'rte_eal',
  'rte_telemetry',
  'rte_bus_pci',
  'rte_pci',
  'rte_ring',
  'rte_mempool',
  'rte_kvargs'
] : []

isal_libnames = get_option('build_subprojects') ? [
 'isal',
] : []

cc = meson.get_compiler('c')
spdk_paths = []
spdk_libs = []
foreach libname : wpdk_libnames + spdk_libnames + dpdk_libnames
  lib_dep = cc.find_library(
    libname,
    dirs: [
      meson.current_source_dir() / 'build' / 'lib',
      meson.current_source_dir() / 'dpdk' / 'build' / 'lib',
      meson.current_source_dir() / '..' / 'wpdk' / 'build' / 'lib',
    ],
    static: true
  )
  spdk_libs += lib_dep

  # Create a bunch of paths
  paths = [
    meson.current_source_dir() / 'build' / 'lib' / 'lib' + libname + '.a',
    meson.current_source_dir() / 'dpdk' / 'build' / 'lib' / 'lib' + libname + '.a',
    meson.current_source_dir() / '..' / 'wpdk' / 'build' / 'lib' / 'lib' + libname + '.a',
  ]
  foreach path : paths
    if lib_dep.found() and fs.exists(path)
      spdk_paths += path
    endif
  endforeach
endforeach

spdk_inc = get_option('build_subprojects') ? include_directories(
  'dpdk' / 'build' / 'include',
  '..' / 'wpdk' / 'build' / 'include' / 'wpdk',
  'build' / 'include'
) : include_directories('.')
spdk_link_args = spdk_paths

# Construct link_args based on the above
spdk_win_dep = declare_dependency(
  dependencies: spdk_libs,
  link_args: spdk_link_args,
  include_directories: spdk_inc,
  variables: {'lib_paths': get_option('build_subprojects') ? ' '.join(spdk_paths) : '.'}
)

meson.override_dependency('spdk-win', spdk_win_dep)
