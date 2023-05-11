"""
Install xNVMe Python Packages using source-tarball artifacts from GitHUB
========================================================================

The xNVMe Python packages:

* xnvme-core.tar.gz

Are expected to be available in 'step.with.xnvme_source'. Additionally,
'step.with.xnvme_source' is expected to the be the root of an extracted xNVMe
source archive.

These xNVMe Python package will be injected into a pipx-environment named
'cijoe', thus making cijoe, pytest, and the xNVMe library within this
environment.

Step Arguments
--------------

step.with.source: The files listed above are expected to be available in
'step.with.source', it is also expected that 'step.with.source' is a directory
containing the xNVMe source in extracted form. That is the content of the
xnvme-source-tarball.

Retargetable: True
------------------
"""
import errno
import logging as log


def main(args, cijoe, step):
    """Transfer artifacts"""

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    commands = [
        "pipx install cijoe --include-deps",
        "pipx inject cijoe numpy",
        "pipx inject cijoe xnvme-core.tar.gz",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
