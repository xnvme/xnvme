# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

xnvmelib_source = []

# Private headers, for the library and for the in-tree tests and tools
xnvme_priv_inc = include_directories('core', 'nvme', 'util', 'backend', 'platform')
xnvmelib_inc = [conf_inc, xnvme_inc, xnvme_priv_inc]

subdir('util')
subdir('backend')
subdir('platform')
subdir('core')
subdir('nvme')
subdir('file')
subdir('cli')

xnvmelib_source += xnvme_vcs_h

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
  cuda_dep,
  hip_dep,
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
  include_directories: xnvmelib_inc,
  install_dir: xnvmelib_libdir,
  install: true,
)

subdir('cuse')

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
