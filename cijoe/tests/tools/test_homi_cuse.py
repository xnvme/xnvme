"""
homi exposes each PCIe-attached controller it holds as a CUSE ioctl mimic
under /dev/xnvme/<bdf> (on by default; --no-cuse disables it), backed by the
same lib/xnvme_cuse.c session qublk's /dev/ublkb<dev_id>-ctl uses. This case
starts a homi primary directly (independent of --shm_id) and sends the same
identify-controller command through both nvme-cli and xNVMe's own CLI.
"""

import pytest

from ..conftest import MprocPrimary, get_osname, require_nvme_cli, xnvme_parametrize

CTL_BE = "nil"


def require_cuse(cijoe):
    """Skip unless homi's CUSE mimic (on by default; --no-cuse disables it) can come up"""

    # Linux-only (meson.build); homi also runs on FreeBSD (spdk), which has its own cuse.ko
    if get_osname() != "linux":
        pytest.skip("homi's CUSE mimic is Linux-only")

    cijoe.run("modprobe cuse 2>/dev/null || true")

    err, _ = cijoe.run("test -c /dev/cuse")
    if err:
        pytest.skip("homi's CUSE mimic requires /dev/cuse (modprobe cuse)")


@pytest.fixture(autouse=True)
def homi_cuse_cleanup(cijoe):
    """Skip when the CUSE mimic or nvme-cli are unusable; leave no primary behind"""

    require_cuse(cijoe)
    require_nvme_cli(cijoe)

    yield

    MprocPrimary.stop(cijoe)


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    if not be_opts.get("mproc"):
        pytest.skip(f"{be_opts['be']} does not support multi-process")

    MprocPrimary.start(cijoe, be_opts["be"], be_opts.get("label"))

    ctl = f"/dev/xnvme/{device['uri']}"

    err, _ = cijoe.run(f"test -c {ctl}")
    assert not err

    err, _ = cijoe.run(f"nvme id-ctrl {ctl}")
    assert not err

    err, _ = cijoe.run(f"xnvme idfy-ctrlr {ctl} --be {CTL_BE}")
    assert not err
