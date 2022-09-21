project(
  'fio',
  'c',
  version: '3.32',
)
fs = import('fs')

if get_option('build_subprojects') and not fs.exists('config-host.h')
  message('Configuring ..')
  run_command(['./configure', '--disable-xnvme'], capture: true, check: true)
endif
if get_option('build_subprojects') and not fs.exists('fio')
  cpu_count = run_command([
    import('python').find_installation('python3'),
    '-c',
    'from multiprocessing import cpu_count; print(cpu_count())'
    ],
    check: true,
  ).stdout().strip()

  message('Building ..')
  run_command([
      find_program(host_machine.system() == 'freebsd' ? 'gmake' : 'make'),
      '-j',
      cpu_count
    ],
    capture: true,
    check: true
  )
endif

fio_inc = include_directories('.')
fio_dep = declare_dependency(
  include_directories: fio_inc
)

meson.override_dependency('fio', fio_dep)
