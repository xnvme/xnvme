import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def test_enum(cijoe):
    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("xnvme test kv enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("xnvme test kv enum")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme test kv idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_info_csi_kv(cijoe, device, be_opts, cli_args):
    """A key-value namespace is identified by CSI 0x1 and classified as XNVME_GEO_KV"""

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err, state.output()

    output = state.output()
    lines = [line.strip() for line in output.split("\n")]
    for expected in ["csi: 0x1", "type: XNVME_GEO_KV"]:
        assert expected in lines, f"missing '{expected}' in:\n{output}"


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_delete_store_exist(cijoe, device, be_opts, cli_args):
    key = "hello"
    val = "world"

    # This is just to ensure the key is not there
    cijoe.run(f"xnvme test kv delete {cli_args} --key {key}")

    err, _ = cijoe.run(
        f"xnvme test kv exist {cli_args} --key {key}",
    )
    assert err, "We expected this to fail"

    err, _ = cijoe.run(
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-update",
    )
    assert err, "We expected this to fail"

    commands = [
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-add",
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-update",
        f"xnvme test kv store {cli_args} --key {key} --value {val} --compress",
        f"xnvme test kv store {cli_args} --key {key} --value {val}",
        f"xnvme test kv exist {cli_args} --key {key}",
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
        cijoe.run(f"xnvme test kv delete {cli_args} --key {key}")

    for key, value in pairs:
        err, _ = cijoe.run(
            f"xnvme test kv store {cli_args} --key {key} --value {value}"
        )
        assert not err

    err, _ = cijoe.run(f"xnvme test kv list {cli_args} --key {key}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_retrieve(cijoe, device, be_opts, cli_args):
    key, val = ("hello", "world")

    # This is just to ensure the key is not there
    cijoe.run(f"xnvme test kv delete {cli_args} --key {key}")

    err, _ = cijoe.run(f"xnvme test kv retrieve {cli_args} --key {key}")
    assert err, "We expected this to fail"

    err, _ = cijoe.run(f"xnvme test kv store {cli_args} --key {key} --value {val}")
    assert not err

    err, _ = cijoe.run(f"xnvme test kv retrieve {cli_args} --key {key}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_store_optional(cijoe, device, be_opts, cli_args):
    key = "hello"
    val = "world"
    val_next = "xnvme"

    err, _ = cijoe.run(f"xnvme test kv delete {cli_args} --key {key}")

    err, _ = cijoe.run(
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-update"
    )
    assert err, "We expected this to fail"

    err, _ = cijoe.run(f"xnvme test kv store {cli_args} --key {key} --value {val}")
    assert not err

    err, _ = cijoe.run(
        f"xnvme test kv store {cli_args} --key {key} --value {val_next} --only-update"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-add"
    )
    assert err, "We expected this to fail"

    err, _ = cijoe.run(f"xnvme test kv delete {cli_args} --key {key}")
    assert not err

    err, _ = cijoe.run(
        f"xnvme test kv store {cli_args} --key {key} --value {val} --only-add"
    )
    assert not err
