xnvmelib_source = [
  'xnvme_adm.c',
  'xnvme_be.c',
  'xnvme_be_fbsd.c',
  'xnvme_be_fbsd_dev.c',
  'xnvme_be_fbsd_nvme.c',
  'xnvme_be_fbsd_sync.c',
  'xnvme_be_linux.c',
  'xnvme_be_linux_async_libaio.c',
  'xnvme_be_linux_async_liburing.c',
  'xnvme_be_linux_async_ucmd.c',
  'xnvme_be_linux_block.c',
  'xnvme_be_linux_dev.c',
  'xnvme_be_linux_nvme.c',
  'xnvme_be_nosys.c',
  'xnvme_be_posix.c',
  'xnvme_be_posix_admin.c',
  'xnvme_be_posix_async_aio.c',
  'xnvme_be_posix_async_emu.c',
  'xnvme_be_posix_async_nil.c',
  'xnvme_be_posix_async_thrpool.c',
  'xnvme_be_posix_dev.c',
  'xnvme_be_posix_mem.c',
  'xnvme_be_posix_sync.c',
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
  'xnvme_be_windows.c',
  'xnvme_be_windows_async_iocp.c',
  'xnvme_be_windows_async_iocp_th.c',
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
  'xnvme_sgl.c',
  'xnvme_spec.c',
  'xnvme_spec_pp.c',
  'xnvme_ver.c',
  'xnvme_znd.c',
  'xnvmec.c'
]

xnvmelib_deps_shared = [
  thread_dep,
  rt_dep,
  math_dep,
  numa_dep,
  uuid_dep,
  execinfo_dep,
  elf_dep,
  aio_dep,
  setupapi_dep
]

xnvmelib_deps_static = []
xnvmelib_deps_static += [spdk_dep, uring_dep]
xnvmelib_deps = xnvmelib_deps_shared + xnvmelib_deps_static

xnvmelib_deps_bundle = [
  uring_dep,
  spdk_dep
]

# We are not relying on meson to control whether or not static/shared libraries are built, this is
# since we need the static library for constructing the bundled library, and we want the
# 'non-bundled' shared library to be named 'libxnvme-static.' and the bundled to be 'libxnvme'.
# This naming is not possible when delegating via default_library
if get_option('shared_library')
  xnvmelib_shared = shared_library(
    'xnvme-shared',
    xnvmelib_source,
    dependencies: xnvmelib_deps,
    include_directories: [conf_inc, xnvme_inc],
    install_dir: xnvmelib_libdir,
    install: true,
  )
endif

xnvmelib_static = static_library(
  'xnvme-static',
  xnvmelib_source,
  dependencies: xnvmelib_deps,
  include_directories: [conf_inc, xnvme_inc],
  install_dir: xnvmelib_libdir,
  install: true,
)

# Produce a static library containing xNVMe, liburing, and SPDK
bundle_lib_paths = []
bundle_lib_paths += [ 'lib' / 'libxnvme-static.a' ]
foreach blib : xnvmelib_deps_bundle
  if get_option('build_subprojects') and blib.found()
    bundle_lib_paths += blib.get_variable(internal: 'lib_paths').split(' ')
  endif
endforeach

# missing xnvme and liburing, howto find / produce absolute paths to these?
xnvmelib_bundle = custom_target(
  'xnvme',
  build_by_default: true,
  depends: xnvmelib_static,
  command: [ prog_python, '@INPUT@', '--output', '@OUTPUT@', '--libs' ] + bundle_lib_paths,
  input: tb_library_bundler,
  output: 'libxnvme.a',
  install_dir: xnvmelib_libdir,
  install: true,
)

# We create this as a target to be consumed by pkgconfig, potentially also for the xnvme-cli-tools,
# although they currently link "directly" to the regular static library
xnvmelib_bundle_dep = declare_dependency(
  dependencies: xnvmelib_deps,
  link_args: conf_data.get('XNVME_BE_SPDK_ENABLED') ? [
    '-Wl,--whole-archive',
    '-Wl,--no-as-needed',
    '-lxnvme',
    '-Wl,--no-whole-archive',
    '-Wl,--as-needed',
  ] : [ '-lxnvme' ]
)

# pkg-config -- describing how to consume the xNVMe library
pconf.generate(
  libraries: [xnvmelib_bundle_dep] + [xnvmelib_deps_shared],
  version: meson.project_version(),
  variables: [
    'datadir=' + get_option('prefix') / get_option('datadir') / meson.project_name()
  ],
  name: 'xNVMe',
  filebase: 'xnvme',
  description : 'Libraries and tools for traditional and emerging I/O interfaces'
)
