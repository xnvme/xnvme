from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_write(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_io_async write {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_io_async read {cli_args}")
    assert not err
