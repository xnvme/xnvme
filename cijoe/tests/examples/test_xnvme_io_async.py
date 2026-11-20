from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme example io-async {cli_args} --elba 0x7 --qdepth 4")
    assert not err
