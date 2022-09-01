from ..conftest import XnvmeDriver, xnvme_parametrize


def test_enum(cijoe):

    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("lblk enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("lblk enum")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk idfy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_read(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk read {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_write(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk write {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["write_uncor"], opts=["be", "admin"])
def test_write_uncor(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk write-uncor {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["write_zeroes"], opts=["be", "admin"])
def test_write_zeroes(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"lblk write-zeros {cli_args} --slba 0x0 --nlb 0")
    assert not err
