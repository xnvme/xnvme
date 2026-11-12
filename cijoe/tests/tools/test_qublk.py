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
    oncs,
    passes_nvme,
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
def test_run_user_copy(cijoe, device, be_opts, cli_args):
    """With --user-copy, a random pattern written with direct I/O reads back the same"""

    payload = [
        "src=$(mktemp)",
        "dst=$(mktemp)",
        'trap "rm -f $src $dst" EXIT',
        "dd if=/dev/urandom of=$src bs=1M count=32 status=none",
        f"dd if=$src of={UBLK_NODE} bs=1M oflag=direct status=none",
        f"dd if={UBLK_NODE} of=$dst bs=1M count=32 iflag=direct status=none",
        "cmp $src $dst",
    ]
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        payload,
        args="--qdepth 64 --nqueues 2 --user-copy",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_del_leftover(cijoe, device, be_opts, cli_args):
    """SIGKILL the server, then recover the leftover device with 'xnvme qublk del'"""

    script = "\n".join(
        [
            "set -u",
            "modprobe ublk_drv || echo MODPROBE-FAILED",
            f"if [ -b {UBLK_NODE} ]; then echo PREEXISTING-DEVICE; exit 1; fi",
            "log=$(mktemp)",
            f"xnvme qublk serve {device['uri']} --be {be_opts['be']} --dev-id 0 > $log 2>&1 &",
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
            "xnvme qublk del --dev-id 0",
            "rc=$?",
            "for i in $(seq 1 25); do [ -c /dev/ublkc0 ] || break; sleep 0.2; done",
            "if [ -c /dev/ublkc0 ]; then echo LEFTOVER-DEVICE; rc=1; fi",
            # A nonexistent identifier must fail rather than report success
            "if xnvme qublk del --dev-id 999; then echo DEL-BOGUS-ID-PASSED; rc=1; fi",
            "cat $log",
            "rm -f $log",
            "exit $rc",
        ]
    )
    err, state = cijoe.run(f"bash -c '{script}'")
    assert not err, state.output()


def _reject_at_setup(cijoe, device, be_opts, phrase):
    """Run 'xnvme qublk serve' on a namespace it must refuse; expect 'phrase' in its output"""

    # The rejection comes before ADD_DEV, so a correct qublk exits on its own
    # and the session need not wait for a device; the timeout only bounds a
    # qublk that wrongly went on to serve, and SIGINT lets it tear down
    script = "\n".join(
        [
            "set -u",
            "modprobe ublk_drv || echo MODPROBE-FAILED",
            f"if [ -b {UBLK_NODE} ]; then echo PREEXISTING-DEVICE; exit 1; fi",
            "log=$(mktemp)",
            f"timeout -s INT 10 xnvme qublk serve {device['uri']} --be {be_opts['be']} "
            "--dev-id 0 > $log 2>&1",
            "rc=$?",
            "if [ $rc -eq 0 ]; then echo SERVE-PASSED; rc=1; "
            "elif [ $rc -eq 124 ]; then echo SERVE-TIMED-OUT; rc=1; "
            "else rc=0; fi",
            f'grep -q "{phrase}" $log || {{ echo MISSING-REJECTION; rc=1; }}',
            f"if [ -b {UBLK_NODE} ]; then echo LEFTOVER-DEVICE; rc=1; fi",
            "cat $log",
            "rm -f $log",
            "exit $rc",
        ]
    )
    err, state = cijoe.run(f"bash -c '{script}'")
    assert not err, state.output()


@xnvme_parametrize(labels=["zns"], opts=["be"])
def test_reject_zoned(cijoe, device, be_opts, cli_args):
    """A zoned namespace is refused at setup on non-passthru backends; see test_qublk_zoned.py"""

    if passes_nvme(be_opts):
        pytest.skip(
            f"[be={be_opts['be']}] passes NVMe commands through and serves a zoned "
            "namespace; covered by test_qublk_zoned.py"
        )

    _reject_at_setup(cijoe, device, be_opts, "zoned (ZNS)")


# The name must contain "kv": test IDs carry no device labels (see paramid in
# conftest.py), so CI's tcp-linux "-k 'fabrics and not kv'" and the mproc steps
# can only exclude the key-value cases by name. The kernel nvmet target does not
# export the passthrough KV port 4422, so a case not matching "kv" would be
# selected there and fail to connect.
@xnvme_parametrize(labels=["kvs"], opts=["be"])
def test_reject_kvs(cijoe, device, be_opts, cli_args):
    """A key-value namespace is refused at setup; no block-device is served"""

    _reject_at_setup(cijoe, device, be_opts, "namespace is key-value")


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


UBLK_QUEUE = f"/sys/block/{UBLK_NODE.split('/')[-1]}/queue"
MIB = 1 << 20


def _advertised(cijoe, device, be_opts):
    """What qublk is expected to advertise, as (discard, write_zeroes)"""

    if not passes_nvme(be_opts):
        return False, False

    return oncs(cijoe, device["uri"], be_opts)


def _fails_unsupported(command, marker, message="Operation not supported"):
    """
    Shell lines asserting that 'command' fails with EOPNOTSUPP, which the tool
    reports as 'message'; the message tells it from a refusal for another
    reason, and LC_ALL=C keeps it untranslated
    """

    return [
        f"out=$(LC_ALL=C {command} 2>&1) && {{ echo {marker}; exit 1; }}",
        'echo "$out"',
        f'echo "$out" | grep -q "{message}"',
    ]


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_discard(cijoe, device, be_opts, cli_args):
    """
    Discard is advertised exactly when the backend passes NVMe commands through
    and the controller lists Dataset Management; then blkdiscard succeeds, else
    the kernel refuses it

    What a deallocated block reads as is up to the namespace (DLFEAT), so the
    data is not checked here; see test_run_write_zeroes for the read-back.
    """

    discard, _ = _advertised(cijoe, device, be_opts)

    payload = [
        f"lbs=$(cat {UBLK_QUEUE}/logical_block_size)",
        f"dmb=$(cat {UBLK_QUEUE}/discard_max_bytes)",
        f"dg=$(cat {UBLK_QUEUE}/discard_granularity)",
        'echo "discard_max_bytes=$dmb discard_granularity=$dg logical_block_size=$lbs"',
    ]
    # --force: blkdiscard declines a device carrying a filesystem signature,
    # which a previous case's mkfs or the random pattern can leave
    blkdiscard = f"blkdiscard -f --offset 0 --length {MIB} {UBLK_NODE}"
    if discard:
        payload += [
            "[ $dmb -gt 0 ]",
            "[ $((dmb % lbs)) -eq 0 ]",
            "[ $dg -eq $lbs ]",
            f"dd if=/dev/urandom of={UBLK_NODE} bs=1M count=1 oflag=direct status=none",
            blkdiscard,
        ]
    else:
        # The granularity is then the kernel's default, not qublk's
        payload += [
            "[ $dmb -eq 0 ]",
            *_fails_unsupported(blkdiscard, "DISCARD-PASSED"),
        ]

    err, state = qublk_session(cijoe, device["uri"], be_opts["be"], payload)
    assert not err, state.output()


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write_zeroes(cijoe, device, be_opts, cli_args):
    """
    Write zeroes is advertised exactly when the backend passes NVMe commands
    through and the controller lists Write Zeroes

    blkdiscard -z (BLKZEROOUT) reads back as zeroes either way, since the
    kernel falls back to writing zeroed pages when nothing is advertised; a
    punch-hole fallocate insists on the offload, so it succeeds exactly when
    advertised and is refused otherwise.
    """

    _, write_zeroes = _advertised(cijoe, device, be_opts)

    def zeroed_after(command):
        return [
            f"dd if=/dev/urandom of={UBLK_NODE} bs=1M count=1 oflag=direct status=none",
            command,
            f"dd if={UBLK_NODE} of=$got bs=1M count=1 iflag=direct status=none",
            "cmp $zero $got",
        ]

    punch_hole = f"fallocate --punch-hole --offset 0 --length {MIB} {UBLK_NODE}"

    payload = [
        f"lbs=$(cat {UBLK_QUEUE}/logical_block_size)",
        f"wzmb=$(cat {UBLK_QUEUE}/write_zeroes_max_bytes)",
        'echo "write_zeroes_max_bytes=$wzmb logical_block_size=$lbs"',
        "zero=$(mktemp)",
        "got=$(mktemp)",
        'trap "rm -f $zero $got" EXIT',
        "dd if=/dev/zero of=$zero bs=1M count=1 status=none",
        *zeroed_after(f"blkdiscard -f -z --offset 0 --length {MIB} {UBLK_NODE}"),
    ]
    if write_zeroes:
        # One Write Zeroes covers at most 65536 blocks, its 16-bit count
        payload += [
            "[ $wzmb -gt 0 ]",
            "[ $wzmb -le $((65536 * lbs)) ]",
            *zeroed_after(punch_hole),
        ]
    else:
        payload += [
            "[ $wzmb -eq 0 ]",
            # fallocate(1) words the kernel's EOPNOTSUPP after the
            # FALLOC_FL_KEEP_SIZE it adds to a punch hole
            *_fails_unsupported(
                punch_hole, "PUNCH-HOLE-PASSED", "keep size mode is unsupported"
            ),
        ]

    err, state = qublk_session(cijoe, device["uri"], be_opts["be"], payload)
    assert not err, state.output()


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        f"'{device['uri']}/?nsid={device['nsid']}'",
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=4k count=64 iflag=direct"],
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
