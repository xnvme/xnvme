"""
Prepare auxilary xNVMe files
============================

This populates "/opt/fio_custom" with:

* fio
  - sets it executable
* external fio-engine: spdk_bdev
* external fio-engine: spdk_nvme

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


def worklet_entry(args, cijoe, step):

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    commands = [
        "ldconfig /usr/local/lib/ || true",
        "xnvme enum",
        "xnvme library-info",
        "mkdir -p /opt/fio_custom",
        "cp subprojects/fio/fio /opt/fio_custom/",
        "cp subprojects/spdk/build/fio/spdk_bdev /opt/fio_custom/",
        "cp subprojects/spdk/build/fio/spdk_nvme /opt/fio_custom/",
        "find /opt/fio_custom",
        "chmod +x /opt/fio_custom/fio",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
