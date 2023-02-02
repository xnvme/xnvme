xnvmelib_source = [
  'xnvme_adm.c',
  'xnvme_be.c',
  'xnvme_be_cbi_admin_shim.c',
  'xnvme_be_cbi_async_emu.c',
  'xnvme_be_cbi_async_nil.c',
  'xnvme_be_cbi_async_posix.c',
  'xnvme_be_cbi_async_thrpool.c',
  'xnvme_be_cbi_mem_posix.c',
  'xnvme_be_cbi_sync_psync.c',
  'xnvme_be_fbsd.c',
  'xnvme_be_fbsd_dev.c',
  'xnvme_be_fbsd_nvme.c',
  'xnvme_be_linux.c',
  'xnvme_be_linux_async_libaio.c',
  'xnvme_be_linux_async_liburing.c',
  'xnvme_be_linux_async_ucmd.c',
  'xnvme_be_linux_block.c',
  'xnvme_be_linux_dev.c',
  'xnvme_be_linux_hugepage.c',
  'xnvme_be_linux_nvme.c',
  'xnvme_be_macos.c',
  'xnvme_be_macos_admin.c',
  'xnvme_be_macos_dev.c',
  'xnvme_be_macos_sync.c',
  'xnvme_be_nosys.c',
  'xnvme_be_ramdisk.c',
  'xnvme_be_ramdisk_admin.c',
  'xnvme_be_ramdisk_dev.c',
  'xnvme_be_ramdisk_sync.c',
  'xnvme_be_spdk.c',
  'xnvme_be_spdk_admin.c',
  'xnvme_be_spdk_async.c',
  'xnvme_be_spdk_dev.c',
  'xnvme_be_spdk_mem.c',
  'xnvme_be_spdk_sync.c',
  'xnvme_be_vfio.c',
  'xnvme_be_vfio_admin.c',
  'xnvme_be_vfio_async.c',
  'xnvme_be_vfio_dev.c',
  'xnvme_be_vfio_mem.c',
  'xnvme_be_vfio_sync.c',
  'xnvme_be_windows.c',
  'xnvme_be_windows_async_iocp.c',
  'xnvme_be_windows_async_iocp_th.c',
  'xnvme_be_windows_async_ioring.c',
  'xnvme_be_windows_block.c',
  'xnvme_be_windows_dev.c',
  'xnvme_be_windows_fs.c',
  'xnvme_be_windows_mem.c',
  'xnvme_be_windows_nvme.c',
  'xnvme_buf.c',
  'xnvme_cmd.c',
  'xnvme_dev.c',
  'xnvme_file.c',
  'xnvme_geo.c',
  'xnvme_ident.c',
  'xnvme_lba.c',
  'xnvme_libconf.c',
  'xnvme_libconf_entries.c',
  'xnvme_nvm.c',
  'xnvme_opts.c',
  'xnvme_queue.c',
  'xnvme_req.c',
  'xnvme_spec.c',
  'xnvme_spec_pp.c',
  'xnvme_ver.c',
  'xnvme_znd.c',
  'xnvmec.c'
]

pconf = import('pkgconfig')
pconf_bin = find_program('pkg-config')


#
# Third-party dependencies
#
thread_dep = dependency('threads')

fio_cfg_h = 'subprojects'/'fio'/'config-host.h'
fio_dep = dependency(
  'fio',
  required: get_option('with-fio')
)
# Fio Windows Posix Headers
if is_windows and get_option('with-fio')
    fio_winposix_inc = include_directories('subprojects'/'fio'/'os'/'windows'/'posix'/'include')
else
    fio_winposix_inc = include_directories()
endif

setupapi_dep = cc.find_library(
  'setupapi',
  has_headers: ['setupapi.h'],
  required: conf_data.get('XNVME_BE_WINDOWS_ENABLED'),
)

rt_dep = cc.find_library('rt', required: is_darwin ? false : (is_linux or is_freebsd))
if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
  error('clock_gettime not found')
endif

aio_dep = cc.find_library(
  'aio',
  has_headers: ['libaio.h'],
  required: conf_data.get('XNVME_BE_LINUX_LIBAIO_ENABLED'),
)

uring_dep = dependency(
  'liburing',
  version: '>=2.2',
  required: false,
)
if conf_data.get('XNVME_BE_LINUX_LIBURING_ENABLED') and not uring_dep.found()
  error('missing liburing >= 2.2; install it or point PKG_CONFIG_PATH to where it is located')
endif

dpdk_dep = dependency(
  'libdpdk',
  required: false,
)

if conf_data.get('XNVME_BE_SPDK_ENABLED') and not dpdk_dep.found()
   error('missing libdpdk; install it or point PKG_CONFIG_PATH to where it is located')
endif

spdk_nvme_dep = dependency(
  'spdk_nvme',
  required: false,
)

if conf_data.get('XNVME_BE_SPDK_ENABLED') and not spdk_nvme_dep.found()
   error('missing spdk_nvme; install it or point PKG_CONFIG_PATH to where it is located')
endif

spdk_env_dpdk_dep = dependency(
  'spdk_env_dpdk',
  required:false,
)

if conf_data.get('XNVME_BE_SPDK_ENABLED') and not spdk_env_dpdk_dep.found()
   error('missing spdk_env_dpdk; install it or point PKG_CONFIG_PATH to where it is located')
endif

spdk_syslibs_dep = dependency(
  'spdk_syslibs',
  required:false,
)

if conf_data.get('XNVME_BE_SPDK_ENABLED') and not spdk_syslibs_dep.found()
   error('missing spdk_syslibs; install it or point PKG_CONFIG_PATH to where it is located')
endif


spdk_nvme_ldflags_cmd = run_command(pconf_bin, '--libs', 'spdk_nvme', check: true)
spdk_env_dpdk_ldflags_cmd = run_command(pconf_bin, '--libs', 'spdk_env_dpdk', check: true)
spdk_syslibs_ldflags_cmd = run_command(pconf_bin, '--libs', '--static', 'spdk_syslibs', check: true)

spdk_nvme_ldflags = spdk_nvme_ldflags_cmd.stdout().split() 
spdk_env_dpdk_ldflags = spdk_env_dpdk_ldflags_cmd.stdout().split()
spdk_syslibs_ldflags = spdk_syslibs_ldflags_cmd.stdout().split()

spdk_static_ldflags = ['-Wl,--whole-archive'] + spdk_nvme_ldflags + spdk_env_dpdk_ldflags + spdk_syslibs_ldflags + ['-Wl,--no-whole-archive']

spdk_shared_ldflags = ['-Wl,--no-as-needed'] + spdk_nvme_ldflags + spdk_env_dpdk_ldflags + ['-Wl,--no-as-needed']

corefoundation_dep = dependency(
  'appleframeworks',
  modules: 'CoreFoundation',
  required: is_darwin
)
iokit_dep = dependency(
  'appleframeworks',
  modules: 'IOKit',
  required: is_darwin
)
libvfn_dep = dependency(
  '_vfn',
  default_options: [
    'libnvme=disabled',
    'docs=disabled',
    'default_library=static',
  ],
  required: conf_data.get('XNVME_BE_LINUX_VFIO_ENABLED')
)


xnvmelib_deps = [aio_dep, corefoundation_dep, iokit_dep, libvfn_dep, rt_dep, setupapi_dep, thread_dep, uring_dep]

xnvme_static_lib = static_library(
  'xnvme',
  xnvmelib_source,
  dependencies: xnvmelib_deps,
  include_directories: [conf_inc, xnvme_inc],
  link_args: spdk_static_ldflags,
  install_dir: xnvmelib_libdir,
  install: true,
)
xnvme_static_dep = declare_dependency(
  include_directories: [conf_inc, xnvme_inc],
  link_args: spdk_static_ldflags,
  dependencies: xnvmelib_deps,
)

xnvme_shared_lib = shared_library(
  'xnvme',
  xnvmelib_source,
  dependencies: xnvmelib_deps,
  include_directories: [conf_inc, xnvme_inc],
  link_args: spdk_shared_ldflags,
  install_dir: xnvmelib_libdir,
  install: true,
)
xnvme_shared_dep = declare_dependency(
  link_with: xnvme_shared_lib,
  include_directories: [conf_inc, xnvme_inc],
  link_args: spdk_shared_ldflags,
  dependencies: xnvmelib_deps,
)

# Define variables for use when linking executables
set_variable('lib_static', xnvme_static_dep)
set_variable('lib_shared', xnvme_shared_dep)

# pkg-config -- describing how to consume the xNVMe library
pconf.generate(
  libraries: xnvme_shared_lib,
  libraries_private: ['-Wl,--whole-archive'] + [xnvme_static_lib] + ['-Wl,--no-whole-archive'],
  version: meson.project_version(),
  variables: [
    'datadir=' + get_option('prefix') / get_option('datadir') / meson.project_name()
  ],
  name: 'xNVMe',
  filebase: 'xnvme',
  description : 'Libraries and tools for traditional and emerging I/O interfaces'
)
