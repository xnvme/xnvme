# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

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
  'xnvme_be_fbsd_async.c',
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
  'xnvme_cli.c',
  'xnvme_cmd.c',
  'xnvme_dev.c',
  'xnvme_file.c',
  'xnvme_geo.c',
  'xnvme_ident.c',
  'xnvme_kvs.c',
  'xnvme_lba.c',
  'xnvme_libconf.c',
  'xnvme_libconf_entries.c',
  'xnvme_nvm.c',
  'xnvme_opts.c',
  'xnvme_queue.c',
  'xnvme_req.c',
  'xnvme_spec.c',
  'xnvme_spec_pp.c',
  'xnvme_topology.c',
  'xnvme_ver.c',
  'xnvme_znd.c',
  'xnvme_crc.c',
  'xnvme_pi.c',
]

xnvmelib_deps = [
  setupapi_dep,
  corefoundation_dep,
  iokit_dep,
  isal_dep,
  libvfn_dep,
  uring_dep,
  aio_dep,
  spdk_dep,
  thread_dep,
  rt_dep,
]

mapfile = 'libxnvme.map'
# Version script is not supported by the linker on OSX
# Use --undefined-version to avoid errors with linkers that have --no-undefined-version by default,
# i.e. ld.lld on newer FreeBSD
version_script = is_darwin ? [] : ['-Wl,--version-script=@0@/@1@'.format(meson.current_source_dir(), mapfile)]

if is_freebsd
  version_script += '-Wl,--undefined-version'
endif

# We always build both the shared and static libraries
xnvmelib = library(
  'xnvme',
  xnvmelib_source,
  version: meson.project_version(),
  dependencies: xnvmelib_deps,
  link_args: link_args_hardening + version_script,
  link_depends: mapfile,
  include_directories: [conf_inc, xnvme_inc],
  install_dir: xnvmelib_libdir,
  install: true,
)


# pkg-config -- describing how to consume the xNVMe library
pconf.generate(
  xnvmelib,
  install_dir: is_freebsd ? 'libdata/pkgconfig' : xnvmelib_libdir + '/pkgconfig',
  version: meson.project_version(),
  variables: [
    'datadir=' + get_option('prefix') / get_option('datadir') / meson.project_name()
  ],
  name: 'xNVMe',
  filebase: 'xnvme',
  description : 'Libraries and tools for traditional and emerging I/O interfaces'
)
