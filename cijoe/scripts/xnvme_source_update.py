"""
Update a provisioned xNVMe source tree with a local working tree
================================================================

Archives the local tree's tracked files and its untracked ones that git does
not ignore, transfers the archive and extracts it over 'xnvme_source', which a
full provision left with its subprojects and builddir. Files removed locally
are not removed there; a full provision starts over.

Retargetable: True
------------------
"""

import errno
import logging as log
import shlex
import tempfile
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    parser.add_argument(
        "--xnvme_source",
        type=str,
        required=True,
        help="path to the provisioned xNVMe source on the target",
    )
    parser.add_argument(
        "--tree",
        type=str,
        default="..",
        help="path to the local working tree to update it with",
    )


def main(args, cijoe):
    """Archive the local tree and extract it over the provisioned one"""

    tree = Path(args.tree).resolve()

    with tempfile.TemporaryDirectory() as tmp:
        listing = Path(tmp) / "files.list"
        deleted = Path(tmp) / "deleted.list"
        archive = Path(tmp) / "xnvme-update.tar.gz"

        err, state = cijoe.run_local(
            "git ls-files -z --cached --others --exclude-standard"
            f" > {shlex.quote(str(listing))}",
            cwd=str(tree),
        )
        if err:
            log.error(f"git ls-files in {tree}: {state.output().strip()}")
            return errno.EINVAL

        err, state = cijoe.run_local(
            f"git ls-files -z --deleted > {shlex.quote(str(deleted))}", cwd=str(tree)
        )
        if err:
            log.error(f"git ls-files in {tree}: {state.output().strip()}")
            return errno.EINVAL
        gone = set(deleted.read_bytes().split(b"\0"))
        files = [f for f in listing.read_bytes().split(b"\0") if f and f not in gone]
        listing.write_bytes(b"\0".join(files))

        err, state = cijoe.run_local(
            f"tar --null -czf {shlex.quote(str(archive))}"
            f" -T {shlex.quote(str(listing))}",
            cwd=str(tree),
        )
        if err:
            log.error(f"tar: {state.output().strip()}")
            return errno.EIO

        err, _ = cijoe.run(f"test -d {args.xnvme_source}/builddir")
        if err:
            log.error(f"no builddir in {args.xnvme_source}; provision the target first")
            return errno.ENOENT
        if not cijoe.put(str(archive), f"{args.xnvme_source}/xnvme-update.tar.gz"):
            return errno.EIO

    err, _ = cijoe.run(
        "tar xzf xnvme-update.tar.gz && rm xnvme-update.tar.gz", cwd=args.xnvme_source
    )
    return err
