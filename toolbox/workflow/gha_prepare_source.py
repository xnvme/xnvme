"""
Prepare xNVMe source directory using GitHUB Artifacts
=====================================================

xNVMe GitHUB artifacts:

* xnvme-core.tar.gz
* xnvme-cy-bindings.tar.gz
* xnvme-cy-header.tar.gz
* xnvme.tar.gz

Transfers the above artifacts from step.with.artifacts, to step.with.xnvme_sources and
extracts the content of xnvme.tar.gz

Step Arguments
--------------

step.with.artifacts: path to where the GitHUB artifacts are located locally
step.with.xnvme_source: path to where the GitHUB artifacts should transferred to

Retargetable: True
------------------
"""
import logging as log


def worklet_entry(args, cijoe, step):
    """Transfer artifacts"""

    artifacts = step.get("with", {}).get("artifacts", "/tmp/artifacts")
    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")

    err, _ = cijoe.run(f"mkdir {xnvme_source}")
    if err:
        return err

    files = [
        "xnvme-core.tar.gz",
        "xnvme-cy-bindings.tar.gz",
        "xnvme-cy-header.tar.gz",
        "xnvme.tar.gz",
    ]
    for filename in files:
        cijoe.put(f"{artifacts}/{filename}", f"{xnvme_source}/{filename}")

    commands = [
        f"ls -lh {xnvme_source}",
        "tar xzf xnvme.tar.gz --strip 1",
        "rm xnvme.tar.gz",
        "df -h",
        "ls -lh",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
