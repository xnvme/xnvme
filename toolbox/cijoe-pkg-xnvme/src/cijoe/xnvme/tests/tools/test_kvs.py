import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize

pytest.skip(allow_module_level=True, reason="Not implemented")


def test_enum(cijoe):

    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("kvs enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("kvs enum")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"kvs info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"kvs idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_delete_store_exist(cijoe, device, be_opts, cli_args):

    key = "hello"
    val = "world"

    # This is just to ensure the key is not there
    cijoe.run(f"kvs delete {cli_args} --key {key}")

    commands = [
        f"kvs exist {cli_args} --key {key}",
        f"kvs store {cli_args} --key {key} --value {val}",
        f"kvs exist {cli_args} --key {key}",
    ]
    for command in commands:
        err, _ = cijoe.run(command)
        assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_delete_store_list(cijoe, device, be_opts, cli_args):

    pairs = [
        ("hello", "world"),
        ("marco", "polo"),
    ]

    # This is just to ensure the key is not there
    for key, value in pairs:
        cijoe.run(f"kvs delete {cli_args} --key {key}")

    for key, value in pairs:
        err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {value}")
        assert not err

    err, _ = cijoe.run(f"kvs list {cli_args} --key {key}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_retrieve(cijoe, device, be_opts, cli_args):

    key, val = ("hello", "world")

    # This is just to ensure the key is not there
    cijoe.run(f"kvs delete {cli_args} --key {key}")

    err, _ = cijoe.run(f"kvs retrieve {cli_args} --key {key}")
    assert not err

    err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {val}")
    assert not err

    err, _ = cijoe.run(f"kvs retrieve {cli_args} --key {key}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_store_optional(cijoe, device, be_opts, cli_args):

    key = "hello"
    val = "world"
    val_next = "xnvme"

    err, _ = cijoe.run(f"kvs delete {cli_args} --key {key}")

    err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {val} --only-update")
    assert not err

    err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {val}")
    assert not err

    err, _ = cijoe.run(
        f"kvs store {cli_args} --key {key} --value {val_next} --only-update"
    )
    assert not err

    err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {val} --only-add")
    assert err

    err, _ = cijoe.run(f"kvs delete {cli_args} --key {key}")
    assert not err

    err, _ = cijoe.run(f"kvs store {cli_args} --key {key} --value {val} --only-add")
    assert not err
