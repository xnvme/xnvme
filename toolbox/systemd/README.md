# systemd units

`xnvme-homi@.service.in` runs a HOMI server, one instance per `homi_id`, with
`homi.conf.example` as the configuration to copy into `/etc/xnvme/`.

It is a template rather than a finished unit: `@bindir@` is substituted at
configuration time so the installed unit names the `homi` from the same build.
Read the generated unit from the build directory, not this file, when checking
what a given build installs.

`xnvme-qublk@.service.in` is the same thing for the `qublk` ublk server, one
instance per ublk device id, with `qublk.conf.example` naming the backing
device: `systemctl start xnvme-qublk@0` serves `/dev/ublkb0`. It reports
started once the block device node appears, so a client can do I/O the moment
`systemctl start` returns.

The build installs both files by default where systemd is present, which
`-Dsystemd=disabled` turns off and `-Dsystemd_unitdir=` relocates. Setup,
multiple instances, and probing a running server are documented under
`docs/tools/homi/`, which is where to look rather than here.
