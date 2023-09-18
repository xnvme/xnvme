project(
  'spdk',
  'c',
  version: '23.09',
)
fs = import('fs')

system_support = ['freebsd', 'linux', 'windows', 'darwin']
system = host_machine.system()
message('host_machine.system:', system)
if not system_support.contains(system)
    error('Unsupported system type "@0@"'.format(exec_env))
endif
foreach sys : system_support
    set_variable('is_' + sys, system == sys)
endforeach

is_centos7 = is_linux ? run_command('cat', '/etc/os-release', check: false).stdout().contains('CentOS Linux 7') : false

cc = meson.get_compiler('c')

math_dep = cc.find_library(
  'm',
  has_headers: ['math.h'],
)

ssl_dep = dependency(
  is_centos7 ? 'openssl11' : 'openssl', version: '>=1.1.1',
)

dlfcn_dep = cc.find_library(
  'dl',
  has_headers: ['dlfcn.h'],
)

uuid_dep = cc.find_library(
  'uuid',
  dirs: is_freebsd ? [ '/usr/local/lib' ] : [],
  header_include_directories: is_freebsd ? [ include_directories('/usr/local/include') ] : [],
  has_headers: ['uuid/uuid.h'],
)
numa_dep = cc.find_library(
  'numa',
  has_headers: ['numaif.h'],
  required: is_linux,
)
execinfo_dep = cc.find_library(
  'execinfo',
  has_headers: ['execinfo.h'],
  required: is_freebsd,
)
elf_dep = cc.find_library(
  'elf',
  has_headers: ['sys/elf_common.h'],
  required: is_freebsd,
)

archive_dep = cc.find_library(
  'archive',
  has_headers: ['archive.h'],
)

if get_option('build_subprojects') and not fs.exists('build')
  message('Patching ..')
  run_command('spdk_patches.sh', capture: true, check: true)
  message('Configuring ..')
  if get_option('buildtype') == 'debug'
     run_command('spdk_configure_debug.sh', capture: true, check: true)
  else
    run_command('spdk_configure.sh', capture: true, check: true)
  endif
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

if get_option('build_subprojects') and host_machine.system() != 'freebsd'
   spdk_libnames += 'spdk_vfio_user'
endif

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

spdk_deps = [dlfcn_dep, math_dep, numa_dep, uuid_dep, ssl_dep, ssl_dep, execinfo_dep, elf_dep, archive_dep,]

spdk_paths = []
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
spdk_link_args = ['-Wl,--whole-archive'] + spdk_paths + ['-Wl,--no-whole-archive']

# Construct link_args based on the above
spdk_dep = declare_dependency(
  dependencies: spdk_deps,
  link_args: spdk_link_args,
  include_directories: spdk_inc,
)
