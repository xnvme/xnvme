"""
qublk session-helper
====================

qublk is a blocking daemon; it serves a ublk block-device until signalled. Exercising it
therefore cannot be done by running a command and inspecting its status, the way the
one-shot command-line tools are tested. Instead, each test-case runs a single shell
session which launches qublk in the background, waits for the block-device to appear,
runs a payload against it, and tears it down with SIGINT.

* qublk_session(cijoe, uri, be, payload, ...): run 'payload' against a served device

* qublk_teardown(cijoe, mountpoint): remove leftovers from a failed test-case

Three things the session-script must get right, each of which has bitten before:

* The lines are joined by newline, not by ';'. The background launch already ends in
  '&', and a ';' following it is a shell syntax error.

* The qublk log goes to a mktemp path. A fixed path in /tmp breaks the next run as soon
  as one is left behind by another user.

* The payload runs in a subshell with 'set -e', so the first failing line decides the
  status. Without it a multi-line payload would report only its last line, and e.g. a
  failed mkfs followed by a successful mount would pass. The subshell keeps 'set -e'
  away from the teardown, which must run regardless.
"""

import pytest

UBLK_NODE = "/dev/ublkb0"
UBLK_CONTROL = "/dev/ublk-control"

DEFAULT_TIMEOUT_TICKS = 50  # 0.2s each, so 10s for the block-device to appear


def require_ublk(cijoe):
    """
    Skip unless ublk can actually be used

    qublk needs root and the 'ublk_drv' module. Neither is available in the unprivileged
    containers running the ramdisk test-suite, and those select on the 'ramdisk' keyword,
    which matches the ramdisk-backend parametrization of these very test-cases. Without
    this guard they would be collected there and fail for reasons having nothing to do
    with qublk.
    """

    err, _ = cijoe.run("test $(id -u) -eq 0")
    if err:
        pytest.skip("qublk requires root")

    cijoe.run("modprobe ublk_drv 2>/dev/null || true")

    err, _ = cijoe.run(f"test -c {UBLK_CONTROL}")
    if err:
        pytest.skip(f"qublk requires the ublk_drv module ({UBLK_CONTROL} is absent)")


def require_mkfs_xfs(cijoe):
    """Skip unless mkfs.xfs is available; the fs-level cases format with XFS"""

    err, _ = cijoe.run("command -v mkfs.xfs")
    if err:
        pytest.skip("the filesystem-level cases require mkfs.xfs (xfsprogs)")


def qublk_script(uri, be, args, payload, node=UBLK_NODE, mountpoint=None):
    """
    Produce the shell session serving 'node' from the given device

    'payload' is a list of shell lines run while the block-device is served; its exit status
    becomes the exit status of the session. When 'mountpoint' is given it is unmounted during
    teardown regardless of what the payload did.
    """

    umount = [
        f"umount {mountpoint} 2>/dev/null || umount -l {mountpoint} 2>/dev/null || true"
    ]

    return "\n".join(
        [
            "set -u",
            "id -un",
            "modprobe ublk_drv || echo MODPROBE-FAILED",
            # A leftover device from a crashed run would satisfy the readiness
            # wait while the qublk under test got another id; refuse to start
            f"if [ -b {node} ]; then echo PREEXISTING-DEVICE; exit 1; fi",
            "log=$(mktemp)",
            f"qublk run {uri} --be {be} --dev-id 0 {args} > $log 2>&1 &".replace(
                "  >", " >"
            ),
            "pid=$!",
            f"for i in $(seq 1 {DEFAULT_TIMEOUT_TICKS}); "
            f"do [ -b {node} ] && break; sleep 0.2; done",
            f"if [ ! -b {node} ]; then echo MISSING-DEVICE; cat $log; "
            "kill -INT $pid 2>/dev/null; exit 1; fi",
            "(",
            "set -e",
            *payload,
            ")",
            "rc=$?",
            *(umount if mountpoint else []),
            "kill -INT $pid 2>/dev/null",
            "wait $pid",
            f"if [ -b {node} ]; then echo LEFTOVER-DEVICE; rc=1; fi",
            "cat $log",
            "rm -f $log",
            "exit $rc",
        ]
    )


def qublk_session(cijoe, uri, be, payload, args="", node=UBLK_NODE, mountpoint=None):
    """Run 'payload' against the ublk block-device served by qublk"""

    script = qublk_script(uri, be, args, payload, node=node, mountpoint=mountpoint)

    return cijoe.run(f"bash -c '{script}'")


def qublk_teardown(cijoe, mountpoint=None, node=UBLK_NODE):
    """Ensure a failed test-case does not leave a mount or a ublk device behind"""

    if mountpoint:
        cijoe.run(
            f"umount {mountpoint} 2>/dev/null || umount -l {mountpoint} 2>/dev/null || true"
        )

    cijoe.run("pkill -INT qublk || true")
    cijoe.run(f"for i in $(seq 1 25); do [ -b {node} ] || break; sleep 0.2; done")
