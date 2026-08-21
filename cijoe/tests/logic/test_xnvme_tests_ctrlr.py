from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_gfeat_nqueues(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ctrlr gfeat-nqueues {cli_args}")
    assert not err
