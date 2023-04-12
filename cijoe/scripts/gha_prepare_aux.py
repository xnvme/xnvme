"""
Auxiliary preperation
=====================

* Fiddle with ldconfig
* Check whether xNVMe binaries are available
* Checkout and install fio v3.34

Step Arguments
--------------

step.with.xnvme_source: The files listed above are expected to be available in
'step.with.xnvme_source', it is also expected that 'step.with.xnvme_source' is a directory
containing the xNVMe source in extracted form. That is the content of the
xnvme-source-tarball.

Retargetable: True
------------------
"""
import errno
import logging as log


def main(args, cijoe, step):
    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    misc = [
        "ldconfig /usr/local/lib/ || true",
        "xnvme enum",
        "xnvme library-info",
        "git clone https://github.com/axboe/fio.git /tmp/fio",
    ]
    fio = [
        "git checkout fio-3.34",
        "./configure --prefix=/opt/fio",
        "make install || gmake install",
    ]

    for commands, cwd in [(misc, xnvme_source), (fio, "/tmp/fio")]:
        first_err = 0
        for command in commands:
            err, _ = cijoe.run(command, cwd=cwd)
            if err:
                log.error(f"cmd({command}), err({err})")
                first_err = first_err if first_err else err

    return first_err
