import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin"])
def test_log_fdp(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement fdp log pages")

    err, _ = cijoe.run(f"xnvme log-fdp-config {cli_args} --lsi 0x1 --data-nbytes 512")
    assert not err

    err, _ = cijoe.run(f"xnvme log-fdp-stats {cli_args} --lsi 0x1")
    assert not err

    err, _ = cijoe.run(f"xnvme log-ruhu {cli_args} --lsi 0x1 --limit 256")
    assert not err

    err, _ = cijoe.run(
        # Get host events
        f"xnvme log-fdp-events {cli_args} --nsid {device['nsid']} --lsi 0x1 --lsp 0x1 "
        f"--limit 8"
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin"])
def test_feature_set_fdp_events(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement feature-get")

    # Enable all FDP events (0xFF events on placement handle 0)
    err, _ = cijoe.run(
        f"xnvme set-fdp-events {cli_args} --fid 0x1e --feat 0xFF0000 --cdw12 0x1"
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin"])
def test_feature_get_fdp(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement feature-get")

    # Get feature FDP (Endurance group id 0x1 on qemu)
    err, _ = cijoe.run(f"xnvme feature-get {cli_args} --fid 0x1d --cdw11 0x1")
    assert not err

    # Get feature FDP events (0xFF events for placement handle 0)
    err, _ = cijoe.run(
        f"xnvme feature-get {cli_args} --fid 0x1e --cdw11 0xFF0000 --data-nbytes 512"
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin", "sync"])
def test_ruhs(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "fbsd" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="ENOSYS: sync=[psync] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme fdp-ruhs {cli_args} --limit 256")
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin", "sync"])
def test_write_dir(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do write with directives")

    # Write (DPD) to placement id 0x0
    err, _ = cijoe.run(
        f"lblk write-dir {cli_args} --slba 0x0 --nlb 2 --dtype 0x2 --dspec 0x0"
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin", "sync"])
def test_ruhu(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "fbsd" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="ENOSYS: sync=[psync] cannot do mgmt send/receive")

    # Reclaim unit handle update for Placement id 0x0)
    err, _ = cijoe.run(f"xnvme fdp-ruhu {cli_args} --pid 0x0")
    assert not err
