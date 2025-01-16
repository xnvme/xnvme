# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

project(
  'xnvme',
  'c',
  version: '0.7.5',
  license: 'BSD-3-Clause',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'buildtype=release',
    'default_library=both',
    'b_staticpic=true'
  ],
  meson_version: '>=0.58'
)

pconf = import('pkgconfig')
fs = import('fs')

#
# Pre-flight check, operating system and compiler arguments
#
system_support = ['freebsd', 'linux', 'windows', 'darwin']
system = host_machine.system()
message('host_machine.system:', system)
if not system_support.contains(system)
    error('Unsupported system type "@0@"'.format(exec_env))
endif
foreach sys : system_support
    set_variable('is_' + sys, system == sys)
endforeach

cc = meson.get_compiler('c')
cc_flags = [
  '-Wno-missing-braces',
  '-Wno-cast-function-type',
  '-Wno-strict-aliasing'
]

add_project_arguments(
  cc.get_supported_arguments(cc_flags),
  language: 'c'
)

add_project_arguments('-Db_lto='+ (not is_windows).to_string(), language: 'c')
add_project_arguments('-Db_pie='+ get_option('hardening').to_string(), language: 'c')

link_args_hardening = get_option('hardening') ? cc.get_supported_arguments([
  '-znow',
  '-zrelro',
]) : []

xnvmelib_libdir = get_option('libdir')
xnvmelib_rpath = get_option('prefix') + '/' + xnvmelib_libdir

#
# The project itself
#
project_version_major = meson.project_version().split('.')[0]
project_version_minor = meson.project_version().split('.')[1]
project_version_patch = meson.project_version().split('.')[2]

#
# Configuration
#
conf_data = configuration_data()
conf_data.set('XNVME_VERSION_MAJOR', project_version_major)
conf_data.set('XNVME_VERSION_MINOR', project_version_minor)
conf_data.set('XNVME_VERSION_PATCH', project_version_patch)

conf_data.set('XNVME_DEBUG_ENABLED', get_option('buildtype') == 'debug')

conf_data.set('XNVME_BE_CBI_ADMIN_SHIM_ENABLED', get_option('cbi_admin_shim'))
conf_data.set('XNVME_BE_CBI_ASYNC_EMU_ENABLED', get_option('cbi_async_emu'))
conf_data.set('XNVME_BE_CBI_ASYNC_NIL_ENABLED', get_option('cbi_async_nil'))
conf_data.set('XNVME_BE_CBI_ASYNC_POSIX_ENABLED', get_option('cbi_async_posix') and (is_linux or is_freebsd or is_darwin))

thread_dep = dependency(
  'threads',
  required: get_option('cbi_async_thrpool'),
)
conf_data.set('XNVME_BE_CBI_ASYNC_THRPOOL_ENABLED', thread_dep.found())

conf_data.set('XNVME_BE_CBI_MEM_POSIX_ENABLED', get_option('cbi_mem_posix') and (is_linux or is_freebsd or is_darwin))
conf_data.set('XNVME_BE_CBI_SYNC_PSYNC_ENABLED', get_option('cbi_sync_psync') and (is_linux or is_freebsd or is_darwin))

conf_data.set('XNVME_BE_RAMDISK_ENABLED', get_option('be_ramdisk'))

setupapi_dep = cc.find_library(
  'setupapi',
  has_headers: ['setupapi.h'],
  required: is_windows,
)
conf_data.set('XNVME_BE_WINDOWS_ENABLED', setupapi_dep.found())
conf_data.set('XNVME_BE_WINDOWS_FS_ENABLED', conf_data.get('XNVME_BE_WINDOWS_ENABLED'))
conf_data.set('XNVME_BE_WINDOWS_ASYNC_ENABLED', conf_data.get('XNVME_BE_WINDOWS_ENABLED'))

conf_data.set('XNVME_BE_LINUX_ENABLED', is_linux)
conf_data.set('XNVME_BE_LINUX_BLOCK_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)
conf_data.set('XNVME_BE_LINUX_BLOCK_ZONED_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)

aio_dep = cc.find_library(
  'aio',
  has_headers: ['libaio.h'],
  required: get_option('with-libaio'),
)
conf_data.set('XNVME_BE_LINUX_LIBAIO_ENABLED', aio_dep.found())

uring_dep = dependency(
  'liburing',
  version: '>=2.2',
  required: get_option('with-liburing'),
)
conf_data.set('XNVME_BE_LINUX_LIBURING_ENABLED', uring_dep.found())

libvfn_dep = dependency(
  'libvfn',
  version: '>=5.1.0',
  required: get_option('with-libvfn')
)
conf_data.set('XNVME_BE_LINUX_VFIO_ENABLED', libvfn_dep.found())

isal_dep = dependency(
  'libisal',
  version: '>=2.30',
  required: get_option('with-isal'),
)
conf_data.set('XNVME_BE_LINUX_LIBISAL_ENABLED', isal_dep.found())

if (is_linux or is_freebsd)
  spdk_proj = subproject('spdk', required: get_option('with-spdk'))
  spdk_dep = spdk_proj.found() ? spdk_proj.get_variable('spdk_dep') : dependency('', required: false)
else
  spdk_dep = dependency('', required: false)
endif
conf_data.set('XNVME_BE_SPDK_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_FC_ENABLED', spdk_dep.found())

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
conf_data.set('XNVME_BE_MACOS_ENABLED', corefoundation_dep.found() and iokit_dep.found())

rt_dep = cc.find_library('rt', required: is_darwin ? false : (is_linux or is_freebsd))
if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
  error('clock_gettime not found')
endif

conf_data.set('XNVME_BE_FBSD_ENABLED', is_freebsd)

conf = configure_file(
  configuration : conf_data,
  output : 'xnvme_config.h',
)
conf_inc = [ include_directories('.') ]
if is_windows
  conf_inc += [ include_directories('toolbox' / 'third-party' / 'windows') ]
endif
if is_darwin
  conf_inc += [ include_directories('toolbox' / 'third-party' / 'macos') ]
endif

rpm_conf = configuration_data()
rpm_conf.set('NAME',    meson.project_name())
rpm_conf.set('VERSION', meson.project_version())
rpm_conf.set('LICENSE', meson.project_license()[0])
configure_file(
    input:         'xnvme.spec.in',
    output:        'xnvme.spec',
    configuration: rpm_conf,
)

# Provide configuration via header-file 'xnvme_config.h'
add_project_arguments(['-include', 'xnvme_config.h'], language: 'c')

# bash-completions: run the Makefile target 'make gen-bash-completions' to update the completions
bash_completion_dep = dependency(
  'bash-completion',
  version: '>=2.0',
  required: false
)
if bash_completion_dep.found()
  bash_completions_dir = bash_completion_dep.get_variable('completionsdir')
  message('Bash-completions: ' + bash_completions_dir)
  install_completions = true
elif get_option('force_completions') and not is_windows
  # Install completions at default locations if bash-completion is not available on the system
  if is_darwin
     bash_completions_dir = get_option('prefix') / 'share/bash-completion/completions'
  elif is_freebsd
     bash_completions_dir = get_option('prefix') / 'share/bash-completion/completions'
  elif is_linux
     bash_completions_dir = '/usr/share/bash-completion/completions'
  endif

  message('Bash-completions: ' + bash_completions_dir)
  install_completions = true
else
  install_completions = false
endif

# Scripts
subdir('toolbox')

# Headers
subdir('include')

# Library source
subdir('lib')

# Tests, tools, and code examples
# NOTE: to update the man-pages, run the Makefile target 'make gen-man-pages'
if get_option('tests')
  subdir('tests')
  subdir('man/tests')
  if install_completions
     subdir('toolbox/bash_completion.d/tests')
  endif
endif
if get_option('tools')
  subdir('tools')
  subdir('man/tools')
  if install_completions
     subdir('toolbox/bash_completion.d/tools')
  endif
endif
if get_option('examples')
  subdir('examples')
  subdir('man/examples')
  if install_completions
     subdir('toolbox/bash_completion.d/examples')
  endif
endif
