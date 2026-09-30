"""
qublk exposes an xNVMe device as a Linux ublk block-device.

Unlike the other command-line tools, qublk is a blocking daemon; it requires root and
the 'ublk_drv' module, and it runs until signalled. It therefore cannot be exercised by
a single command returning a status; see qublk_session.py for how a test-case drives it.

The cases here exercise the raw block-device; filesystem-level coverage is in
test_qublk_fs.py.
"""

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import (
    UBLK_NODE,
    UBLK_NVME,
    is_userspace_nvme,
    qublk_session,
    qublk_teardown,
    require_cuse,
    require_ublk,
)


@pytest.fixture(autouse=True)
def qublk_cleanup(cijoe):
    """Skip when ublk is unusable; leave no daemon or ublk device behind"""

    require_ublk(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_dd(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=1M count=32 iflag=direct"],
        args="--qdepth 64",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_multi_queue(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=1M count=32 iflag=direct"],
        args="--qdepth 64 --nqueues 4",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_del_leftover(cijoe, device, be_opts, cli_args):
    """SIGKILL the server, then recover the leftover device with 'qublk del'"""

    script = "\n".join(
        [
            "set -u",
            "modprobe ublk_drv || echo MODPROBE-FAILED",
            f"if [ -b {UBLK_NODE} ]; then echo PREEXISTING-DEVICE; exit 1; fi",
            "log=$(mktemp)",
            f"qublk run {device['uri']} --be {be_opts['be']} --dev-id 0 > $log 2>&1 &",
            "pid=$!",
            f"for i in $(seq 1 50); do [ -b {UBLK_NODE} ] && break; sleep 0.2; done",
            f"if [ ! -b {UBLK_NODE} ]; then echo MISSING-DEVICE; cat $log; "
            "kill -INT $pid 2>/dev/null; exit 1; fi",
            # The node exists before the kernel is done with the new disk:
            # add_disk() creates it and then reads its partition table, and
            # that read goes through the ublk queue from the server's own
            # io-wq thread, since START_DEV is a uring_cmd punted there. A
            # server killed inside that window cannot serve the read, its
            # exit waits for the thread, the thread waits for the read, and
            # udev's probe waits for the disk: D state until the guest is
            # rebooted. Wait for udev, whose probe queues behind the scan,
            # and open the node once ourselves before pulling the plug.
            "udevadm settle --timeout=10 || true",
            f"blockdev --getsize64 {UBLK_NODE} > /dev/null",
            "kill -KILL $pid",
            "wait $pid 2>/dev/null",
            # The control-side device outlives the killed server; 'del' must
            # remove it, and the char-device is the observable for that
            "qublk del --dev-id 0",
            "rc=$?",
            "for i in $(seq 1 25); do [ -c /dev/ublkc0 ] || break; sleep 0.2; done",
            "if [ -c /dev/ublkc0 ]; then echo LEFTOVER-DEVICE; rc=1; fi",
            # A nonexistent identifier must fail rather than report success
            "if qublk del --dev-id 999; then echo DEL-BOGUS-ID-PASSED; rc=1; fi",
            "cat $log",
            "rm -f $log",
            "exit $rc",
        ]
    )
    err, state = cijoe.run(f"bash -c '{script}'")
    assert not err, state.output()


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_max_io_bytes(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=128k count=64 iflag=direct"],
        args="--qdepth 64 --max-io-bytes 131072",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_no_nvme_device(cijoe, device, be_opts, cli_args):
    """A device not on a user-space NVMe driver gets no /dev/ublkb<N>-nvme"""

    if is_userspace_nvme(be_opts["be"]):
        pytest.skip(f"[be={be_opts['be']}] gets /dev/ublkb<N>-nvme")
    require_cuse(cijoe)

    # qublk starts the CUSE device before the block device, so once the
    # session's readiness wait has seen the block device, a CUSE device that
    # was going to appear already has
    err, _ = qublk_session(
        cijoe, device["uri"], be_opts["be"], [f"test ! -e {UBLK_NVME}"]
    )
    assert not err
