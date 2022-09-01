from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_hw(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_hello hw {cli_args}")
    assert not err
