"""
Install xNVMe Python Package using source-tarball artifacts from GitHUB
========================================================================

The xNVMe Python package:

* artifacts/xnvme-py-sdist.tar.gz

Is expected to be available in 'step.with.xnvme_source'. Additionally,
'step.with.xnvme_source' is expected to be the root of an extracted xNVMe
source archive.

The xNVMe Python package will be injected into a pipx-environment named
'cijoe', thus making cijoe, pytest, and the xNVMe library available within this
environment.

Step Arguments
--------------

step.with.source: The file listed above is expected to be available in
'step.with.source', it is also expected that 'step.with.source' is a directory
containing the xNVMe source in extracted form. That is the content of the
xnvme-source-tarball.

Retargetable: True
------------------
"""

import errno
import logging as log


def main(args, cijoe, step):
    """Setup cijoe pipx-env with numpy and xNVMe Python bindings injected"""

    osname = cijoe.config.options.get("os", {}).get("name", "")
    if osname not in ["debian"]:
        log.info("skip python setup")
        return 0

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    # NOTE: because pipx (specifically the userpath module it uses) only adds
    # PATH to files read by login-shells, then we cannot use 'pipx ensurepath'
    # anymore. Issue has been reported here:
    #
    # * https://github.com/pypa/pipx/pull/1021
    # * https://github.com/ofek/userpath/issues/50
    #
    # In some scenarios, like pipx via system packages, then older versions
    # would still work, then it is not a problem, however, a recent pip-install
    # pipx will fetch the latest userpath and fail.
    command = "echo $PATH"
    err, state = cijoe.run(command)
    if err:
        log.error(f"cmd({command}), err({err})")
        return err

    if "/root/.local/bin" not in state.output():
        command = "echo 'export PATH=\"$PATH:/root/.local/bin\"' >> .bashrc"
        err, _ = cijoe.run(command)
        if err:
            log.error(f"cmd({command}), err({err})")
            return err

    commands = [
        "pipx ensurepath",
        "pipx install cijoe==v0.9.34 --include-deps",
        "pipx inject cijoe numpy",
        "pipx inject cijoe ./xnvme-py-sdist.tar.gz",
        "cijoe --version",
        "pytest --version",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
