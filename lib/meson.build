# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

# The library is built from three parts: the core, the command-line library and
# the printers of the specification. Each is a static library of its own, linked
# whole into libxnvme, so the parts can become libraries of their own
xnvmelib_source = []
xnvme_cli_source = []
xnvme_pp_source = []

xnvme_priv_inc = include_directories('core', 'nvme', 'util', 'backend', 'platform')
xnvmelib_inc = [conf_inc, xnvme_inc, xnvme_priv_inc]

subdir('util')
subdir('backend')
subdir('platform')
subdir('core')
subdir('nvme')
subdir('file')
subdir('cli')
subdir('pp')

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
  upcie_dep,
]

# The parts take only what compiling needs. pkg-config gathers the link
# arguments of every library linked whole, so a part with the full deps
# repeats SPDK's --whole-archive and its archives define each symbol again
xnvmelib_compile_deps = []
foreach dep : xnvmelib_deps
  xnvmelib_compile_deps += dep.partial_dependency(compile_args: true, includes: true, sources: true)
endforeach

mapfile = 'libxnvme.map'
# Version script is not supported by the linker on OSX
# Use --undefined-version to avoid errors with linkers that have --no-undefined-version by default,
# i.e. ld.lld on newer FreeBSD
version_script = is_darwin ? [] : ['-Wl,--version-script=@0@/@1@'.format(meson.current_source_dir(), mapfile)]

if is_freebsd
  version_script += '-Wl,--undefined-version'
endif

xnvme_core_part = static_library(
  'xnvme-core',
  xnvmelib_source,
  dependencies: xnvmelib_compile_deps,
  include_directories: xnvmelib_inc,
  cuda_args: cuda_arch_args,
  install: false,
)

xnvme_cli_part = static_library(
  'xnvme-cli',
  xnvme_cli_source,
  dependencies: xnvmelib_compile_deps,
  include_directories: xnvmelib_inc,
  install: false,
)

xnvme_pp_part = static_library(
  'xnvme-pp',
  xnvme_pp_source,
  dependencies: xnvmelib_compile_deps,
  include_directories: xnvmelib_inc,
  install: false,
)

# We always build both the shared and static libraries
xnvmelib = library(
  'xnvme',
  link_whole: [xnvme_core_part, xnvme_cli_part, xnvme_pp_part],
  version: meson.project_version(),
  dependencies: xnvmelib_deps,
  link_args: link_args_hardening + version_script,
  link_depends: mapfile,
  link_language: conf_data.get('XNVME_BE_UPCIE_CUDA_ENABLED') ? 'cpp' : 'c',
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
