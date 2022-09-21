project(
  'spdk',
  'c',
  version: '22.05',
)
fs = import('fs')

if get_option('build_subprojects') and not fs.exists('build')
  message('Patching ..')
  run_command('spdk_patches.sh', capture: true, check: true)
  message('Configuring ..')
  run_command('spdk_configure.sh', capture: true, check: true)
endif
if get_option('build_subprojects') and not fs.exists('build' / 'lib' / 'src' / 'spdk_nvme.a')
  cpu_count = run_command([
    import('python').find_installation('python3'),
    '-c',
    'from multiprocessing import cpu_count; print(cpu_count())'
    ],
    check: true,
  ).stdout().strip()

  message('Checking if generic platform (Apple Silicon)')
  is_platform_generic = target_machine.cpu_family() == 'aarch64' and run_command([
    import('python').find_installation('python3'),
    '-c',
    'print(open("/sys/devices/system/cpu/cpu0/regs/identification/midr_el1").read() == "")'
    ],
    check: true,
  ).stdout().strip() == 'True'

  message('Building ..')
  run_command([
      find_program(host_machine.system() == 'freebsd' ? 'gmake' : 'make'),
      '-j',
      cpu_count
    ],
    capture: true,
    check: true,
    env: is_platform_generic ? {'DPDKBUILD_FLAGS':'-Dplatform=generic'} : {}
  )
endif

message('Setting up dependency ..')

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
foreach libname : spdk_libnames + dpdk_libnames + isal_libnames
  lib_dep = cc.find_library(
    libname,
    dirs: [
      meson.current_source_dir() / 'build' / 'lib',
      meson.current_source_dir() / 'dpdk' / 'build' / 'lib',
      meson.current_source_dir() / 'isa-l' / '.libs',
    ],
    static: true
  )
  spdk_libs += lib_dep

  # Create a bunch of paths
  paths = [
    meson.current_source_dir() / 'build' / 'lib' / 'lib' + libname + '.a',
    meson.current_source_dir() / 'dpdk' / 'build' / 'lib' / 'lib' + libname + '.a',
    meson.current_source_dir() / 'isa-l' / '.libs' / 'lib' + libname + '.a',
  ]
  foreach path : paths
    if lib_dep.found() and fs.exists(path)
      spdk_paths += path
    endif
  endforeach
endforeach

spdk_inc = get_option('build_subprojects') ? include_directories(
  'dpdk' / 'build' / 'include',
  'build' / 'include'
) : include_directories('.')
spdk_link_args = ['-Wl,--whole-archive', '-Wl,--no-as-needed'] + spdk_paths + ['-Wl,--no-whole-archive', '-Wl,--as-needed']

# Construct link_args based on the above
spdk_dep = declare_dependency(
  dependencies: spdk_libs,
  link_args: spdk_link_args,
  include_directories: spdk_inc,
  variables: {'lib_paths': get_option('build_subprojects') ? ' '.join(spdk_paths) : '.'}
)

meson.override_dependency('spdk', spdk_dep)
