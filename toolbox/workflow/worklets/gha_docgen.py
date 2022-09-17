"""
Prepare for documentation generation
====================================

Install requirements/dependencies, check binaries and library paths

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
from pathlib import Path


def worklet_entry(args, cijoe, step):

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    commands = [
        "./docs/autogen/debian-bullseye.sh",
        "cd docs/autogen && make deps-system",
        "cd subprojects/fio && make install",
        "xnvme enum",
        "xnvme library-info",
        "pkg-config xnvme --variable=libdir",
        "pkg-config xnvme --variable=datadir",
        "pkg-config xnvme --variable=includedir",
        "sync; echo 1 > /proc/sys/vm/drop_caches",
        "cd docs/autogen && make commands",
        "cd docs/autogen && make docs",
        "cd docs/autogen/builddir/ && tar -czf docs.tar.gz html/.",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    artifacts = Path(cijoe.output_path) / "artifacts"
    cijoe.run_local(f"mkdir -p {artifacts}")
    cijoe.get(
        f"{xnvme_source}/docs/autogen/builddir/docs.tar.gz", f"{artifacts}/docs.tar.gz"
    )

    return first_err
