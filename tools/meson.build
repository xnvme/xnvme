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
  ],
}

tools_bins = []
libexec_names = [] # The programs installed for the front-end to run, as each is defined
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
  else
    libexec_names += bin_name
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

# 'xnvme <command>' runs the program xnvme-<command> only where xnvme does not implement
# <command> itself, so no program installed for it may take the name of one. These are
# g_cli_builtins and then g_subs of tools/xnvme.c, which the test 'front-end, built-ins' holds
# the list to.
frontend_builtins = [
  'env', 'list', 'driver', 'cmd', 'cmd-admin', 'cmd-io', 'cmd-iov', 'test', 'example', 'enum',
  'scan', 'info',
]
foreach name : libexec_names
  if frontend_builtins.contains(name.substring(6))
    error('@0@ would be run as \'xnvme @1@\', which is a command of xnvme itself'.format(
      name, name.substring(6)))
  endif
endforeach

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
  ['list', ['list'], false],
  ['list, bad argument', ['list', '--yaml'], true],
  ['driver', ['driver'], false],
  ['driver --help', ['driver', '--help'], false],
  ['driver, a controller and no backend', ['driver', '0000:ff:1f.7'], true],
  ['driver, no such backend', ['driver', '--be', 'nosuch', '--all'], true],
  ['driver, a backend of no controller', ['driver', '--be', 'ramdisk', '--all'], true],
  ['driver, userspace, nothing named', ['driver', '--be', 'upcie'], true],
  ['driver, userspace, not a controller', ['driver', '--be', 'upcie', '0000:ff:1f.7'], true],
  ['driver, userspace --all, dry-run', ['driver', '--be', 'upcie', '--all', '--dry-run'], false],
  ['driver, kernel, not a controller', ['driver', '--be', 'posix', 'ff:1f.7'], true],
  ['driver, kernel --all, dry-run', ['driver', '--be', 'posix', '--all', '--dry-run'], false],
  ['group cmd', ['cmd'], false],
  ['cmd-admin, identify', ['cmd-admin', '1GB', '--opcode=0x06', '--cdw10=1', '--data-len=4096', '-r'], false],
  ['cmd-admin, short options, binary', ['cmd-admin', '1GB', '-O', '0x06', '-4', '1', '-l', '4096', '-r', '-b'], false],
  ['cmd-admin, dry-run', ['cmd-admin', '1GB', '-O', '0x06', '-n', '1', '-d'], false],
  ['cmd-io, read', ['cmd-io', '1GB', '--opcode=0x02', '--namespace-id=1', '--data-len=4096', '--read'], false],
  ['cmd-iov, read', ['cmd-iov', '1GB', '-O', '0x02', '-n', '1', '-l', '4096', '--vec-cnt', '4', '-r'], false],
  ['cmd-iov, uneven segments', ['cmd-iov', '1GB', '-O', '0x02', '-l', '4096', '--vec-cnt', '3'], true],
  ['cmd-admin, no opcode', ['cmd-admin', '1GB'], true],
  ['cmd-admin, timeout', ['cmd-admin', '1GB', '-O', '6', '-t', '100'], true],
  ['cmd-admin, json', ['cmd-admin', '1GB', '-O', '6', '-o', 'json'], true],
  ['list the commands', ['--list-cmds'], false],
  ['list the commands of a group', ['--list-cmds=env'], false],
  ['list the commands, no such group', ['--list-cmds=nosuch'], true],
  ['--help on a built-in', ['info', '--help'], false],
  ['--help on a group', ['cmd', '--help'], false],
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
    ['run a test, admin command', ['test', 'adm', 'idfy-ctrlr', '1GB'], false],
    ['run a test, hyphenated', ['test', 'znd-append'], false],
    ['no such test', ['test', 'nosuch'], true],
    ['group test --help', ['test', '--help'], false],
    ['list the test programs', ['--list-cmds=test'], false],
    ['a test program lists its subcommands', ['test', 'adm', '--list-cmds'], false],
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

# With nothing on the exec path, 'xnvme --list-cmds' prints the built-ins alone, in order; an
# empty XNVME_EXEC_PATH means the install directory, so it names one that does not exist
frontend_builtins_env = environment()
frontend_builtins_env.set('XNVME_EXEC_PATH', meson.current_build_dir() / 'no-programs')
test(
  'tools - xnvme front-end, built-ins',
  python3,
  args: [
    '-c',
    'import subprocess, sys; out = subprocess.run([sys.argv[1], "--list-cmds"], '
    + 'capture_output=True, text=True).stdout.split(); sys.exit(out != sys.argv[2:])',
    xnvme_bin,
  ] + frontend_builtins,
  env: frontend_builtins_env,
  suite: 'frontend',
)
