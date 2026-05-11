from ..conftest import xnvme_parametrize


# sync=[block,psync]: compare not supported via block/psync
@xnvme_parametrize(
    labels=["dev"], opts=["be", "admin", "sync"], exclude={"sync": ["block", "psync"]}
)
def test_compare(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_compare compare {cli_args}")
    assert not err
