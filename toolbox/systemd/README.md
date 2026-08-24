# systemd units

`xnvme-homi@.service.in` runs a HOMI primary, one instance per `shm_id`, with
`homi.conf.example` as the configuration to copy into `/etc/xnvme/`.

It is a template rather than a finished unit: `@bindir@` is substituted at
configuration time so the installed unit names the `homi` from the same build.
Read the generated unit from the build directory, not this file, when checking
what a given build installs.

The build installs both files by default where systemd is present, which
`-Dsystemd=disabled` turns off and `-Dsystemd_unitdir=` relocates. Setup,
multiple instances, and probing a running primary are documented under
`docs/tools/homi/`, which is where to look rather than here.
