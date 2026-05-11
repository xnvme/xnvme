from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_io(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_lblk io {cli_args}")
    assert not err


# sync=[block,psync]: write_uncorrectable not supported via block/psync
@xnvme_parametrize(
    labels=["write_uncor"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"]},
)
def test_write_uncorrectable(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_lblk write_uncorrectable {cli_args}")
    assert not err


# sync=[block,psync]: write_zeroes not supported via block/psync
@xnvme_parametrize(
    labels=["write_zeroes"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"]},
)
def test_write_zeroes(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_lblk write_zeroes {cli_args}")
    assert not err
