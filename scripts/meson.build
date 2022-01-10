meson.add_dist_script('meson_dist_archive_fixer.py')

if not is_windows
  xnvmescript_driver = custom_target(
    'xnvme-driver-script',
    input: 'xnvme-driver.sh',
    output: 'xnvme-driver',
    command: [find_program('cp'), '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: get_option('bindir')
  )
endif
