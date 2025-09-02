"""
In a development environment where the project source is edited on a "local"
system, and deployed for build and test on another/different "remote" system.

Then one needs to transfer the source, or changes to the source, to the remote
system. This can be done with tools such as rsync and mutagen. However, these
can cause unwanted behavior when syncing repository data such as ``.git``.

Ideally, one would execute the required git commands, transferring the changes
to the remote, however, this can be tedious. Thus, this script, running the
various commands, is doing just that.

Here is what it does:

* remote: checkout main
* local: commit changes with message "autocommit ..."
* local: push changes to remote
* remote: checkout step.repository.branch

With the above, then git is utilized for syncing, and the cijoe-scripting takes
care of the need to switch branch remotely.
"""

import logging as log
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument("--guest_name", type=str, required=True)


def main(args, cijoe):
    """Entry point"""

    if cijoe.getconf("os.name", None) != "windows":
        log.info("not windows, skipping windows QEMU prep")
        return 0

    # Move the boot image to prevent cijoe in using the VirtIO
    # boot driver.
    path = Path(cijoe.getconf(f"qemu.guests.{args.guest_name}.path")).resolve()
    bootimg_old = path / "boot.img"
    bootimg_new = path / "boot2.img"
    err, _ = cijoe.run_local(f"mv {bootimg_old} {bootimg_new}")
    if err:
        return err

    # Start TPM emulator
    swtpm_socket = path / "swtpm.sock"
    cmds = [
        "sudo pkill swtpm || true",
        f"rm -rf {swtpm_socket}",
        f"swtpm socket --tpm2 --ctrl type=unixio,path={swtpm_socket} --tpmstate dir={path} --flags not-need-init --daemon",
    ]
    for cmd in cmds:
        err, _ = cijoe.run_local(cmd)
        if err:
            return err

    return 0
