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

Retargetable: True
------------------
"""

import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        default="/tmp/xnvme_source",
        help="path to xNVMe source",
    )


def main(args, cijoe):
    """Setup cijoe pipx-env with numpy and xNVMe Python bindings injected"""

    osname = cijoe.getconf("os.name", "")
    if osname not in ["debian"]:
        log.info("skip python setup")
        return 0

    xnvme_source = args.xnvme_source

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
