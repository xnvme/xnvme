import pytest

from ..conftest import xnvme_parametrize

pytest.skip(allow_module_level=True, reason="Not implemented")


def test_xpy_enumerate(cijoe):

    err, _ = cijoe.run("xpy_enumerate")
    assert not err


def test_xpy_libconf(cijoe):

    err, _ = cijoe.run("xpy_libconf")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_xpy_dev_open(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(
        f"xpy_dev_open --uri {cli_args['uri']} --dev-nsid {cli_args['nsid']}"
    )
    assert not err
