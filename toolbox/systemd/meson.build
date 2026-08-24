# The unit runs 'homi', so it is worth nothing without the tools, and systemd
# itself is Linux-only. Ask for it rather than deciding: a feature left at
# 'auto' installs where systemd is present and stays quiet where it is not,
# and -Dsystemd=disabled turns it off for a build that should not touch a
# machine's unit directory.
systemd_opt = get_option('systemd')
systemd_unitdir = ''

if not systemd_opt.disabled()
  systemd_unitdir = get_option('systemd_unitdir')

  # Only ask systemd where units belong when nobody has said. Packaging a
  # build for another root is the case that needs this: it sets the directory
  # and has no reason to have systemd installed to be told the same thing.
  if systemd_unitdir == ''
    systemd_dep = dependency('systemd', required: systemd_opt)
    if systemd_dep.found()
      # This comes back absolute, as /usr/lib/systemd/system, and so ignores
      # the prefix this build was configured with. Redefining prefix keeps a
      # staged or relocated install inside its own tree.
      systemd_unitdir = systemd_dep.get_variable(
        pkgconfig: 'systemdsystemunitdir',
        pkgconfig_define: ['prefix', get_option('prefix')],
      )
    endif
  endif
endif

if systemd_unitdir != ''
  unit_conf = configuration_data()
  # Absolute, since systemd resolves neither PATH nor a relative ExecStart
  unit_conf.set('bindir', get_option('prefix') / get_option('bindir'))

  # The unit has to read its configuration from where this build puts it, or a
  # prefixed install writes the example to one path and reads from another
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
