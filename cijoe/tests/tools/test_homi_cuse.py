"""
homi exposes each PCIe-attached controller it holds as a CUSE ioctl mimic
under /dev/xnvme/<bdf> (on by default; --no-cuse disables it), backed by the
same lib/xnvme_cuse.c session qublk's /dev/ublkb<dev_id>-ctl uses. These
cases start a homi control-plane server directly (independent of --homi-id):
one sends the same identify-controller command through both nvme-cli and
xNVMe's own CLI, the other confirms that namespace-scoped ioctls are declined
on it, since homi holds a controller, not a namespace.
"""

import pytest

from ..conftest import CPlaneServer, get_osname, require_nvme_cli, xnvme_parametrize

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

    CPlaneServer.stop(cijoe)


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    if not be_opts.get("cplane"):
        pytest.skip(f"{be_opts['be']} cannot share a controller")

    CPlaneServer.start(cijoe, be_opts["be"], be_opts.get("label"))

    ctl = f"/dev/xnvme/{device['uri']}"

    err, _ = cijoe.run(f"test -c {ctl}")
    assert not err

    err, _ = cijoe.run(f"nvme id-ctrl {ctl}")
    assert not err

    err, _ = cijoe.run(f"xnvme idfy-ctrlr {ctl} --be {CTL_BE}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_decline_namespace_ioctls(cijoe, device, be_opts, cli_args):
    """homi holds a controller, not a namespace; namespace-scoped ioctls must fail on it"""

    if not be_opts.get("cplane"):
        pytest.skip(f"{be_opts['be']} cannot share a controller")

    CPlaneServer.start(cijoe, be_opts["be"], be_opts.get("label"))

    ctl = f"/dev/xnvme/{device['uri']}"

    # NVME_IOCTL_ID: no namespace of its own to report one for
    err, _ = cijoe.run(f"nvme get-ns-id {ctl}")
    assert err

    # NVME_IOCTL_IO_CMD: no namespace of its own to read from
    err, _ = cijoe.run(f"nvme read {ctl} -n 1 -s 0 -c 0 -z 512 -o binary > /dev/null")
    assert err
