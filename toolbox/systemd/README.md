# systemd units

`xnvme-homi@.service` runs a HOMI primary, one instance per `shm_id`, with
`homi.conf.example` as the configuration to copy into `/etc/xnvme/`.

Setup, multiple instances, and probing a running primary are documented under
`docs/tools/homi/`, which is where to look rather than here.
