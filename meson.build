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
    'b_staticpic=true',
    'b_ndebug=if-release',
  ],
  meson_version: '>=0.59'
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

spdk_rdma_option = get_option('with-spdk-rdma')

if not is_linux and spdk_rdma_option.enabled()
  error('XNVMe with SPDK RDMA support is not currently supported on this system configuration.')
endif

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

if not is_windows
  # '-flto=auto' lets GCC parallelize LTRANS; falls back to bare '-flto'
  # for compilers that do not accept the suffix (e.g. clang).
  flto_args = cc.get_supported_arguments(['-flto=auto'])
  if flto_args.length() == 0
    flto_args = ['-flto']
  endif
  add_project_arguments(flto_args, language: 'c')
  add_project_link_arguments(flto_args, language: 'c')
endif

if get_option('hardening')
  add_project_arguments('-fpie', language: 'c')
  add_project_link_arguments('-pie', language: 'c')
endif

link_args_hardening = get_option('hardening') ? cc.get_supported_arguments([
  '-znow',
  '-zrelro',
]) : []

xnvmelib_libdir = get_option('libdir')
xnvmelib_rpath = get_option('prefix') + '/' + xnvmelib_libdir

#
# The project itself
#
python3 = import('python').find_installation()

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

is_posix = is_linux or is_freebsd or is_darwin
be_posix = get_option('be_posix').require(is_posix, error_message: 'needs Linux, FreeBSD or macOS')
if is_posix and be_posix.disabled()
  error('-Dbe_posix=disabled: the Linux, FreeBSD and macOS backends take their memory and psync from posix')
endif
conf_data.set('XNVME_BE_POSIX_ENABLED', be_posix.allowed())
conf_data.set('XNVME_BE_EMU_ENABLED', get_option('be_emu').allowed())
conf_data.set('XNVME_BE_NIL_ENABLED', get_option('be_nil').allowed())
conf_data.set('XNVME_BE_SHIM_ENABLED', get_option('be_shim').allowed())

thread_dep = dependency('threads')
conf_data.set('XNVME_BE_THRPOOL_ENABLED', get_option('be_thrpool').allowed())

conf_data.set('XNVME_BE_RAMDISK_ENABLED', get_option('be_ramdisk').allowed())
be_upcie = get_option('be_upcie').require(is_linux, error_message: 'needs Linux').allowed()
conf_data.set('XNVME_BE_UPCIE_ENABLED', be_upcie)
if be_upcie
  # uPCIe vendored headers (dmabuf.h: memfd_create + file sealing) need _GNU_SOURCE
  add_project_arguments('-D_GNU_SOURCE', language: 'c')
endif

cuda_dep = dependency(
  'cuda',
  version: '>=12.8',
  modules: ['cuda'],
  required: get_option('with-cuda'),
)
conf_data.set('XNVME_BE_UPCIE_CUDA_ENABLED', be_upcie and cuda_dep.found() and get_option('with-cuda').allowed())
cuda_arch_args = []
if conf_data.get('XNVME_BE_UPCIE_CUDA_ENABLED')
  add_languages('cuda', required: true, native: false)
  cuda_arch_args = ['-arch=native']
endif

# uPCIe HIP/ROCm backend: AMD GPU device memory via the HIP host API (gcc, no
# hipcc), linked against libamdhip64. Mirrors the CUDA backend above.
hip_dep = cc.find_library(
  'amdhip64',
  has_headers: ['hip/hip_runtime.h'],
  header_args: ['-D__HIP_PLATFORM_AMD__'],
  required: get_option('with-hip'),
)
conf_data.set('XNVME_BE_UPCIE_HIP_ENABLED', be_upcie and hip_dep.found() and get_option('with-hip').allowed())
if conf_data.get('XNVME_BE_UPCIE_HIP_ENABLED')
  add_project_arguments('-D__HIP_PLATFORM_AMD__', language: 'c')
endif

setupapi_dep = cc.find_library(
  'setupapi',
  has_headers: ['setupapi.h'],
  required: is_windows,
)
conf_data.set('XNVME_PLATFORM_WINDOWS_ENABLED', setupapi_dep.found())
be_windows = conf_data.get('XNVME_PLATFORM_WINDOWS_ENABLED')
conf_data.set('XNVME_BE_IOCP_ENABLED', get_option('be_iocp').require(be_windows, error_message: 'needs Windows').allowed())
conf_data.set('XNVME_BE_IORING_ENABLED', get_option('be_ioring').require(be_windows, error_message: 'needs Windows').allowed())

xnvme_cuse_has_fuse = is_linux ? cc.has_header('linux/fuse.h', required: get_option('with-cuse')) : false
# Also gate on the 5.17+ NVME_IOCTL_IO64_CMD_VEC symbol, or an older kernel header degrades instead of failing the build.
xnvme_cuse_has_vec = is_linux ? cc.has_header_symbol(
  'linux/nvme_ioctl.h',
  'NVME_IOCTL_IO64_CMD_VEC',
  required: get_option('with-cuse'),
) : false
xnvme_cuse_has_header = xnvme_cuse_has_fuse and xnvme_cuse_has_vec
conf_data.set('XNVME_CUSE_ENABLED', xnvme_cuse_has_header)
if not xnvme_cuse_has_header and is_linux and get_option('with-cuse').allowed()
  warning('No linux/fuse.h or too old linux/nvme_ioctl.h, the NVMe ioctl() CUSE mimic is unavailable')
endif

conf_data.set('XNVME_PLATFORM_LINUX_ENABLED', is_linux)
conf_data.set('XNVME_BE_LINUX_BLOCK_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)
conf_data.set('XNVME_BE_LINUX_BLOCK_ZONED_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)
conf_data.set(
  'XNVME_BE_LINUX_BLK_ZONE_CAPACITY_ENABLED',
  is_linux ? cc.has_member('struct blk_zone', 'capacity', prefix: '#include <linux/blkzoned.h>') : false,
)

be_libaio = get_option('be_libaio')
be_libaio = be_libaio.require(is_linux, error_message: 'reads the Linux backend state')
aio_dep = cc.find_library(
  'aio',
  has_headers: ['libaio.h'],
  required: be_libaio,
)
conf_data.set('XNVME_BE_LIBAIO_ENABLED', aio_dep.found())

be_liburing = get_option('be_liburing')
be_liburing = be_liburing.require(is_linux, error_message: 'reads the Linux backend state')
uring_dep = dependency(
  'liburing',
  version: '>=2.2',
  required: be_liburing,
)
conf_data.set('XNVME_BE_LIBURING_ENABLED', uring_dep.found())

be_libvfn = get_option('be_libvfn')
libvfn_dep = dependency(
  'libvfn',
  version: '>=5.1.0',
  required: be_libvfn
)
conf_data.set('XNVME_BE_LIBVFN_ENABLED', libvfn_dep.found())

isal_dep = dependency(
  'libisal',
  version: '>=2.30',
  required: get_option('with-isal'),
)
conf_data.set('XNVME_LIBISAL_ENABLED', isal_dep.found())

be_spdk = get_option('be_spdk')
if (is_linux or is_freebsd)
  spdk_proj = subproject('spdk', required: be_spdk)
  spdk_dep = spdk_proj.found() ? spdk_proj.get_variable('spdk_dep') : dependency('', required: false)
  spdk_rdma_enabled = spdk_proj.found() ? spdk_proj.get_variable('spdk_rdma_enabled') : false
else
  spdk_dep = dependency('', required: false)
  spdk_rdma_enabled = false
endif
conf_data.set('XNVME_BE_SPDK_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED', spdk_dep.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED', spdk_dep.found() and spdk_rdma_enabled)
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
conf_data.set('XNVME_PLATFORM_MACOS_ENABLED', corefoundation_dep.found() and iokit_dep.found())
conf_data.set('XNVME_BE_DRIVERKIT_ENABLED', get_option('be_driverkit').require(conf_data.get('XNVME_PLATFORM_MACOS_ENABLED'), error_message: 'needs macOS').allowed())

rt_dep = cc.find_library('rt', required: is_darwin ? false : (is_linux or is_freebsd))
if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
  error('clock_gettime not found')
endif

conf_data.set('XNVME_PLATFORM_FREEBSD_ENABLED', is_freebsd)

conf_data.set('XNVME_PTHREAD_SETAFFINITY_NP_ENABLED', cc.has_function(
    'pthread_setaffinity_np',
    prefix: '#define _GNU_SOURCE\n#include <pthread.h>',
    dependencies: thread_dep)
)
conf_data.set('XNVME_RAND_R_ENABLED', cc.has_function('rand_r'))

conf = configure_file(
  configuration : conf_data,
  output : 'xnvme_config.h',
)
conf_inc = [ include_directories('.') ]
if is_linux
  conf_inc += [ include_directories('toolbox' / 'third-party' / 'linux') ]
endif
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
    input:         'toolbox/xnvme.spec.in',
    output:        'xnvme.spec',
    configuration: rpm_conf,
)

# The revision the sources came from, as 'xnvme_vcs.h'
vcs_tag_py = meson.current_source_dir() / 'toolbox' / 'vcs_tag.py'
xnvme_vcs_h = vcs_tag(
  input: 'lib/util/xnvme_vcs.h.in',
  output: 'xnvme_vcs.h',
  command: [python3, vcs_tag_py],
  fallback: 'unknown',
)
meson.add_dist_script(python3, vcs_tag_py, '--dist')

# Provide configuration via header-file 'xnvme_config.h'
add_project_arguments(['-include', 'xnvme_config.h'], language: 'c')
if cuda_dep.found()
  add_project_arguments(['-include', 'xnvme_config.h'], language: 'cuda')
endif

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
