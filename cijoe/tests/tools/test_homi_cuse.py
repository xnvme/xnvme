"""
homi exposes each PCIe-attached controller it holds as a CUSE ioctl mimic
under /dev/xnvme/<bdf> (on by default; --no-cuse disables it), backed by the
same lib/cuse/xnvme_cuse.c session qublk's /dev/ublkb<dev_id>-nvme uses. These
cases start a homi control-plane server directly (independent of --homi-id):
one sends the same identify-controller command through both nvme-cli and
xNVMe's own CLI, the other confirms that namespace-scoped ioctls are declined
on it, since homi holds a controller, not a namespace.
"""

import pytest

from ..conftest import (
    CPlaneServer,
    get_homi_id,
    get_osname,
    require_nvme_cli,
    xnvme_parametrize,
)

CUSE_BE = "nil"


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

    err, _ = cijoe.run(f"xnvme idfy-ctrlr {ctl} --be {CUSE_BE}")
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


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_ioctls_alongside_served_clients(cijoe, device, be_opts, cli_args):
    """The server's heap takes CUSE ioctls and served clients' requests at the same time"""

    if not be_opts.get("cplane"):
        pytest.skip(f"{be_opts['be']} cannot share a controller")
    if not get_homi_id():
        pytest.skip("needs --homi-id: the clients here are served by the server")

    CPlaneServer.start(cijoe, be_opts["be"], be_opts.get("label"))

    ctl = f"/dev/xnvme/{device['uri']}"

    # Every ioctl bounces through a buffer from the server's heap, while one
    # client allocates buffers and another creates queues from the same heap
    err, _ = cijoe.run(
        "set -e; "
        f"( for i in $(seq 1 500); do xnvme_tests_cuse_admin64_identify identify64 {ctl}"
        " > /dev/null; done ) & ioctls=$!; "
        f"xnvme_tests_buf buf_alloc_free_threads {cli_args} --count 2000 & bufs=$!; "
        f"xnvme_tests_threads init_term {cli_args} --count 100 --qdepth 64 & queues=$!; "
        "wait $ioctls; wait $bufs; wait $queues"
    )
    assert not err

    assert CPlaneServer.is_running(cijoe)

    err, _ = cijoe.run(f"nvme id-ctrl {ctl}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_admin_ioctls_alongside_served_admin(cijoe, device, be_opts, cli_args):
    """
    A CUSE admin ioctl and a served client's admin command each get their own completion

    Both reach the controller's one admin queue, from different threads of the
    server. Get Features answers in the completion itself, so each side asks
    for a different feature and checks the answer against one taken alone.
    """

    if not be_opts.get("cplane"):
        pytest.skip(f"{be_opts['be']} cannot share a controller")
    if not get_homi_id():
        pytest.skip("needs --homi-id: the clients here are served by the server")

    CPlaneServer.start(cijoe, be_opts["be"], be_opts.get("label"))

    ctl = f"/dev/xnvme/{device['uri']}"
    ioctl = f"xnvme feature-get {ctl} --be {CUSE_BE} --fid 0x4"
    served = f"xnvme feature-get {cli_args} --fid 0x7"

    err, _ = cijoe.run(
        "set -e; "
        f"ioctl_ref=$({ioctl}); served_ref=$({served}); "
        f'( for i in $(seq 1 500); do test "$({ioctl})" = "$ioctl_ref"; done ) & ioctls=$!; '
        f'( for i in $(seq 1 500); do test "$({served})" = "$served_ref"; done ) & clients=$!; '
        "wait $ioctls; wait $clients"
    )
    assert not err, "an admin command failed or answered with another's completion"
