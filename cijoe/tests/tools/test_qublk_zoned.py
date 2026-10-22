"""
qublk serving a zoned (ZNS) namespace as a host-managed zoned block-device, checked through
the kernel: sysfs, blkzone, fio in zbd mode and btrfs. Backends which do not pass NVMe
commands through are skipped, test_qublk.py::test_reject_zoned covers them.
"""

import re

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import (
    UBLK_NODE,
    passes_nvme,
    qublk_session,
    qublk_teardown,
    require_fio,
    require_mkfs_btrfs,
    require_ublk,
)

MNT = "/mnt/qublk-zoned-test"
QUEUE = f"/sys/block/{UBLK_NODE.split('/')[-1]}/queue"

ZONE_COND_EMPTY = 1
ZONE_COND_EXP_OPEN = 3
ZONE_COND_CLOSED = 4
ZONE_COND_FULL = 14

# Shell helpers: 'zinfo <sector>' prints the zone there as "start len cap wptr cond" in
# decimal; 'zexpect <sector> <want>' fails the payload if that differs from <want>
ZONE_HELPERS = [
    "zinfo() {",
    f"  blkzone report -o $1 -c 1 {UBLK_NODE} | sed 's/,//g; s/zcond:/zcond: /' |",
    "  { read -r _ s _ l _ c _ w _ _ _ cond _;",
    '    echo "$((s)) $((l)) $((c)) $((w)) ${cond%%(*}"; }',
    "}",
    "zexpect() {",
    '  got=$(zinfo "$1")',
    '  if [ "$got" != "$2" ]; then',
    '    echo "zone at sector $1: got [$got] want [$2]"; exit 1',
    "  fi",
    "}",
]

RESET_ALL = f"blkzone reset {UBLK_NODE}"


@pytest.fixture(autouse=True)
def qublk_zoned_cleanup(cijoe, device, be_opts, cli_args):
    """Skip non-passthru combinations; leave no mount or device behind and all zones reset"""

    if not passes_nvme(be_opts):
        pytest.skip(
            f"[be={be_opts['be']}] does not pass NVMe commands through; qublk refuses "
            "a zoned namespace on it, see test_qublk.py::test_reject_zoned"
        )

    require_ublk(cijoe)

    cijoe.run(f"mkdir -p {MNT}")

    yield

    qublk_teardown(cijoe, mountpoint=MNT)

    # A payload that failed midway did not reach its final reset
    cijoe.run(f"zoned mgmt-reset {cli_args} --slba 0 --all")


def zoned_geometry(cijoe, cli_args):
    """(nr_zones, zone_sectors in 512 B sectors), read before serving: user-space drivers do not share"""

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err, state.output()

    geo = {}
    for key in ("nzone", "nsect", "nbytes"):
        match = re.search(rf"^\s*{key}: (\d+)$", state.output(), re.MULTILINE)
        assert match, f"'xnvme info' did not report {key}"
        geo[key] = int(match.group(1))

    assert (geo["nsect"] * geo["nbytes"]) % 512 == 0

    return geo["nzone"], geo["nsect"] * geo["nbytes"] // 512


def kernel_node(device):
    """The namespace's kernel block-device, if it has one, to compare zone reports against"""

    uri = device["uri"]
    if uri.startswith("/dev/nvme"):
        return uri
    if uri.startswith("/dev/ng"):
        return uri.replace("/dev/ng", "/dev/nvme", 1)

    return None


@xnvme_parametrize(labels=["zns"], opts=["be"])
def test_serve_zoned_report(cijoe, device, be_opts, cli_args):
    """Host-managed with the namespace's geometry; full, partial and end-overrunning reports"""

    nr_zones, zone_sectors = zoned_geometry(cijoe, cli_args)
    mid = nr_zones // 2

    payload = [
        f"test $(cat {QUEUE}/zoned) = host-managed",
        f"test $(cat {QUEUE}/nr_zones) -eq {nr_zones}",
        f"test $(cat {QUEUE}/chunk_sectors) -eq {zone_sectors}",
        # Every zone of the full report, in order
        "i=0",
        "while read -r _ s _ l _; do",
        f"  test $((s)) -eq $((i * {zone_sectors})) || {{ echo BAD-START $s; exit 1; }}",
        f"  test $((l)) -eq {zone_sectors} || {{ echo BAD-LEN $l; exit 1; }}",
        "  i=$((i + 1))",
        f"done < <(blkzone report {UBLK_NODE} | sed 's/,//g')",
        f"test $i -eq {nr_zones}",
        # A partial report, and one running over the end
        f"test $(blkzone report -o {mid * zone_sectors} -c 1 {UBLK_NODE} | wc -l) -eq 1",
        f"blkzone report -o {mid * zone_sectors} -c 1 {UBLK_NODE} | sed 's/,//g' | "
        f"{{ read -r _ s _; test $((s)) -eq {mid * zone_sectors}; }}",
        f"test $(blkzone report -o {(nr_zones - 1) * zone_sectors} -c 4 {UBLK_NODE} "
        "| wc -l) -eq 1",
    ]

    node = kernel_node(device)
    if node:
        # The kernel's own report of the namespace is the reference
        payload += [f"diff <(blkzone report {UBLK_NODE}) <(blkzone report {node})"]

    payload += [RESET_ALL]

    err, _ = qublk_session(cijoe, device["uri"], be_opts["be"], payload)
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be"])
def test_serve_zoned_mgmt(cijoe, device, be_opts, cli_args):
    """Zone open, a write advancing the write pointer, close, finish and reset via blkzone"""

    nr_zones, zone_sectors = zoned_geometry(cijoe, cli_args)
    start = (nr_zones // 2) * zone_sectors
    nbytes = 8192
    assert (start * 512) % nbytes == 0

    payload = ZONE_HELPERS + [
        RESET_ALL,
        # The zone capacity is the device's
        f'cap=$(zinfo {start} | cut -d" " -f3)',
        "src=$(mktemp)",
        "dst=$(mktemp)",
        'trap "rm -f $src $dst" EXIT',
        f"dd if=/dev/urandom of=$src bs={nbytes} count=1 status=none",
        f"blkzone open -o {start} -c 1 {UBLK_NODE}",
        f'zexpect {start} "{start} {zone_sectors} $cap 0 {ZONE_COND_EXP_OPEN}"',
        f"dd if=$src of={UBLK_NODE} bs={nbytes} count=1 seek={start * 512 // nbytes} "
        "oflag=direct status=none",
        f"dd if={UBLK_NODE} of=$dst bs={nbytes} count=1 skip={start * 512 // nbytes} "
        "iflag=direct status=none",
        "cmp $src $dst",
        f'test $(zinfo {start} | cut -d" " -f4) -eq {nbytes // 512}',
        f"blkzone close -o {start} -c 1 {UBLK_NODE}",
        f'zexpect {start} "{start} {zone_sectors} $cap {nbytes // 512} {ZONE_COND_CLOSED}"',
        f"blkzone finish -o {start} -c 1 {UBLK_NODE}",
        f'zexpect {start} "{start} {zone_sectors} $cap {zone_sectors} {ZONE_COND_FULL}"',
        f"blkzone reset -o {start} -c 1 {UBLK_NODE}",
        f'zexpect {start} "{start} {zone_sectors} $cap 0 {ZONE_COND_EMPTY}"',
        RESET_ALL,
    ]

    err, _ = qublk_session(cijoe, device["uri"], be_opts["be"], payload)
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be"])
def test_serve_zoned_fio(cijoe, device, be_opts, cli_args):
    """fio in zbd mode: four jobs on their own zones write and verify through four queues"""

    require_fio(cijoe)

    _, zone_sectors = zoned_geometry(cijoe, cli_args)
    zone_bytes = zone_sectors * 512

    payload = [
        RESET_ALL,
        "fio --name=zbd --output-format=terse"
        f" --filename={UBLK_NODE} --ioengine=libaio --zonemode=zbd --direct=1"
        " --rw=write --bs=64k --numjobs=4 --iodepth=8 --verify=crc32c"
        f" --size={2 * zone_bytes} --offset_increment={4 * zone_bytes}",
        RESET_ALL,
    ]

    err, _ = qublk_session(
        cijoe, device["uri"], be_opts["be"], payload, args="--qdepth 64 --nqueues 4"
    )
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be"])
def test_serve_zoned_btrfs(cijoe, device, be_opts, cli_args):
    """Zoned btrfs: write, remount, verify digests, scrub; btrfs uses zone append"""

    require_mkfs_btrfs(cijoe)

    nfiles = 8
    file_mib = 8

    payload = [
        RESET_ALL,
        f"mkfs.btrfs -f -q -O zoned {UBLK_NODE}",
        f"mount {UBLK_NODE} {MNT}",
        f"test $(stat -f -c %T {MNT}) = btrfs",
        f"for i in $(seq 1 {nfiles}); do dd if=/dev/urandom of={MNT}/file$i bs=1M "
        f"count={file_mib} status=none; done",
        f"(cd {MNT} && sha256sum file* > SHA256SUMS)",
        "sync",
        f"umount {MNT}",
        f"mount {UBLK_NODE} {MNT}",
        f"(cd {MNT} && sha256sum -c --quiet SHA256SUMS)",
        f"btrfs scrub start -B {MNT} | tee /dev/stderr | grep -q 'no errors found'",
        f"umount {MNT}",
        RESET_ALL,
    ]

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        payload,
        args="--qdepth 64 --nqueues 4",
        mountpoint=MNT,
    )
    assert not err
