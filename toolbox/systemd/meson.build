systemd_opt = get_option('systemd')
systemd_unitdir = ''

if not systemd_opt.disabled()
  systemd_unitdir = get_option('systemd_unitdir')

  if systemd_unitdir == ''
    systemd_dep = dependency('systemd', required: systemd_opt)
    if systemd_dep.found()
      # pkg-config answers with an absolute path that ignores this build's prefix
      systemd_unitdir = systemd_dep.get_variable(
        pkgconfig: 'systemdsystemunitdir',
        pkgconfig_define: ['prefix', get_option('prefix')],
      )
    endif
  endif
endif

if systemd_unitdir != ''
  unit_conf = configuration_data()
  # Absolute: systemd resolves neither PATH nor a relative ExecStart or EnvironmentFile
  unit_conf.set('bindir', get_option('prefix') / get_option('bindir'))
  unit_conf.set('sysconfdir', get_option('prefix') / get_option('sysconfdir'))

  configure_file(
    input: 'xnvme-homi@.service.in',
    output: 'xnvme-homi@.service',
    configuration: unit_conf,
    install: true,
    install_dir: systemd_unitdir,
  )

  install_data(
    'homi.conf.example',
    install_dir: get_option('sysconfdir') / 'xnvme',
  )

  message('systemd units: ' + systemd_unitdir)
endif
