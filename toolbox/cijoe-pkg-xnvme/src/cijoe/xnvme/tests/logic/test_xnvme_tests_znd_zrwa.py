import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_idfy(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa idfy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_support(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa support {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_open_with_zrwa(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa open-with-zrwa {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_open_without_zrwa(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa open-without-zrwa {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_flush(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_flush_explicit(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush-explicit {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zrwa"], opts=["be", "admin", "sync"])
def test_flush_implicit(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="ENOSYS: sync=[psync,block] cannot do mgmt send/receive")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="ENOSYS: admin=[block] cannot do mgmt send/receive")

    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush-implicit {cli_args}")
    assert not err
