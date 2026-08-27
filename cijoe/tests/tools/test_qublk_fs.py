"""
Filesystem-level coverage for qublk.

qublk serves an xNVMe device as a Linux ublk block-device; these cases put an XFS
filesystem on it and exercise what a block-device is actually used for: formatting,
mounting, file operations, and remounting.

'test_persistence_across_restart' is the case that matters most. Mounting and writing can
succeed while the data never reaches the device; only stopping qublk, starting it again and
comparing the content proves otherwise. It is also what drives qublk's FLUSH and FUA paths.
"""

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import (
    UBLK_NODE,
    qublk_session,
    qublk_teardown,
    require_mkfs_xfs,
    require_ublk,
)

MNT = "/mnt/qublk-test"
PAYLOAD_FILE = f"{MNT}/payload.bin"
PAYLOAD_MIB = 8

MKFS = f"mkfs.xfs -f -q {UBLK_NODE}"
MOUNT = f"mount {UBLK_NODE} {MNT}"
UMOUNT = f"umount {MNT}"

# Write a reproducible payload and record its digest next to it.
# No single-quotes in payload lines: the session is wrapped in bash -c '...'
WRITE_PAYLOAD = [
    f"dd if=/dev/urandom of={PAYLOAD_FILE} bs=1M count={PAYLOAD_MIB} status=none",
    f'sha256sum {PAYLOAD_FILE} | cut -d" " -f1 > {MNT}/payload.sha256',
    "sync",
]

# Compare against the recorded digest; size alone would not catch corrupted content
VERIFY_PAYLOAD = [
    f"test -f {PAYLOAD_FILE}",
    f"test $(stat -c %s {PAYLOAD_FILE}) -eq $(({PAYLOAD_MIB} * 1024 * 1024))",
    f'echo "$(cat {MNT}/payload.sha256)  {PAYLOAD_FILE}" | sha256sum -c -',
]


@pytest.fixture(autouse=True)
def qublk_fs_cleanup(cijoe):
    """Skip when ublk is unusable; leave no mount, daemon or ublk device behind"""

    require_ublk(cijoe)
    require_mkfs_xfs(cijoe)

    cijoe.run(f"mkdir -p {MNT}")

    yield

    qublk_teardown(cijoe, mountpoint=MNT)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_format_mount_umount(cijoe, device, be_opts, cli_args):
    """Format the served device, mount it, confirm the mount, unmount it"""

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            MKFS,
            MOUNT,
            f"findmnt --source {UBLK_NODE} --target {MNT}",
            f"test $(stat -f -c %T {MNT}) = xfs",
            UMOUNT,
        ],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_existing_filesystem(cijoe, device, be_opts, cli_args):
    """Mount a filesystem which is already there, without formatting again"""

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [MKFS, MOUNT, UMOUNT, MOUNT, f"test $(stat -f -c %T {MNT}) = xfs", UMOUNT],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_remount(cijoe, device, be_opts, cli_args):
    """Mount, unmount and mount again within the same qublk session"""

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            MKFS,
            MOUNT,
            f"touch {MNT}/before-remount",
            UMOUNT,
            MOUNT,
            f"test -f {MNT}/before-remount",
            UMOUNT,
        ],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_file_write_read(cijoe, device, be_opts, cli_args):
    """Write a file, read it back and compare the content"""

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [MKFS, MOUNT] + WRITE_PAYLOAD + VERIFY_PAYLOAD + [UMOUNT],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_file_delete(cijoe, device, be_opts, cli_args):
    """Create files and directories, delete them, confirm they are gone"""

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            MKFS,
            MOUNT,
            f"mkdir -p {MNT}/dir/sub",
            f"echo content > {MNT}/dir/sub/file",
            f"test -f {MNT}/dir/sub/file",
            f"rm -r {MNT}/dir",
            f"test ! -e {MNT}/dir",
            UMOUNT,
        ],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_persistence_across_restart(cijoe, device, be_opts, cli_args):
    """Data written in one qublk session must be readable in the next"""

    # The ramdisk backend keeps the device in the memory of the serving process, so its
    # content is gone once qublk stops. That is a property of the backing store, not
    # something qublk could preserve, so the case only applies to persistent devices.
    if "ramdisk" in device["labels"]:
        pytest.skip("ramdisk backing does not persist across a qublk restart")

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [MKFS, MOUNT] + WRITE_PAYLOAD + [UMOUNT],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err, "failed while writing the payload"

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [MOUNT] + VERIFY_PAYLOAD + [UMOUNT],
        args="--qdepth 64",
        mountpoint=MNT,
    )
    assert not err, "payload did not survive the restart"
