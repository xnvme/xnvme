# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tools_source = [
  'homi.c',
  'xnvme.c',
]
tools = {
  'homi.c': [],
  'xnvme.c': [
    ['list', ['list']],
    ['enum', ['enum']],
    ['info', ['info', '1GB']],
    ['idfy, no args', ['idfy', '1GB', '--cns', '0x0']],
    ['idfy, cntid=0x0', ['idfy', '1GB', '--cns', '0x0', '--cntid', '0x0', '--setid', '0x0', '--uuid', '0x0']],
    ['idfy-ns', ['idfy-ns', '1GB']],
    ['idfy-ctrlr', ['idfy-ctrlr', '1GB']],
    ['library-info', ['library-info']],
  ],
}

tools_bins = []
tools_names = {
  'homi.c': 'xnvme-homi',
  'xnvme.c': 'xnvme',
}
foreach source, tests : tools
  bin_name = tools_names[source]
  bin_cuse = source == 'homi.c'
  bin = executable(
    bin_name,
    [source, xnvme_vcs_h],
    include_directories: [conf_inc, xnvme_inc] + (bin_cuse ? [xnvme_cuse_inc] : []),
    link_args: link_args_hardening,
    link_with: [xnvmelib] + (bin_cuse ? [xnvme_cuse_lib] : []),
    install_rpath: xnvmelib_rpath,
    install: true,
    install_dir: bin_name == 'xnvme' ? get_option('bindir') : xnvme_libexecdir,
  )
  if bin_name == 'xnvme'
    xnvme_bin = bin
  endif
  tools_bins += bin
  foreach test : tests
    test_name = test[0]
    test_args = test[1]
    test('tools - '+bin_name+', '+test_name, bin, args : test_args)
  endforeach
endforeach

subdir('xnvmeperf')
subdir('qublk')

# The front-end looks up the programs it runs on XNVME_EXEC_PATH, here the build tree
frontend_env = environment()
frontend_env.set(
  'XNVME_EXEC_PATH',
  (is_windows ? ';' : ':').join([
    meson.project_build_root() / 'tests',
    meson.project_build_root() / 'examples',
    meson.current_build_dir(),
    meson.current_build_dir() / 'xnvmeperf',
    meson.current_build_dir() / 'qublk',
  ]),
)
frontend_tests = [
  ['overview', [], false],
  ['env', ['env'], false],
  ['env system', ['env', 'system'], false],
  ['env library', ['env', 'library'], false],
  ['env, bad argument', ['env', '--yaml'], true],
  ['env, no such section', ['env', 'nosuch'], true],
  ['env --help', ['env', '--help'], false],
  ['--help on a front-end command', ['env', '--help'], false],
  ['group nvme, listing', ['nvme'], false],
  ['group nvme', ['nvme'], false],
  ['--help on a built-in', ['info', '--help'], false],
  ['help on a group', ['help', 'nvme'], false],
  ['exec-path', ['--exec-path'], false],
  ['no such command', ['nosuch'], true],
]
# xnvme-perf is the one program the overview lists, and is built where there are threads
if thread_dep.found()
  frontend_tests += [
    ['--help on a program', ['perf', '--help'], false],
    ['run a program', ['perf'], false],
  ]
endif
if get_option('tests')
  frontend_tests += [
    ['test listing', ['test'], false],
    ['run a test', ['test', 'nvm', 'enum'], false],
    ['run a test, hyphenated', ['test', 'znd-append'], false],
    ['no such test', ['test', 'nosuch'], true],
    ['group test --help', ['test', '--help'], false],
  ]
endif
foreach test : frontend_tests
  test(
    'tools - xnvme front-end, ' + test[0],
    xnvme_bin,
    args: test[1],
    env: frontend_env,
    depends: tools_bins + (get_option('tests') ? tests_bins : []),
    should_fail: test[2],
    suite: 'frontend',
  )
endforeach
