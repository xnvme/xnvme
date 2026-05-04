import pytest

from ..conftest import xnvme_parametrize


def get_alt_be(be: str):
    if be == "upcie":
        return "upcie-cuda"
    elif be == "upcie-cuda":
        return "upcie"
    else:
        return None


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open_dev_twice(cijoe, device, be_opts, cli_args):
    # We are repurposing this test to verify that opening the same device twice with the same backend works
    err, _ = cijoe.run(
        f"xnvme_tests_be_families idfy-cmp {device['uri']} --be {be_opts['be']} --alt-be {be_opts['be']} --dev-nsid {device['nsid']}"
    )
    assert not err


@xnvme_parametrize(labels=["dev", "cuda"], opts=["be"])
def test_io_cmp(cijoe, device, be_opts, cli_args):
    alt_be = get_alt_be(be_opts["be"])
    if not alt_be:
        pytest.skip(reason=f"[be={be_opts['be']}] does not have an alternate backend")
    err, _ = cijoe.run(
        f"xnvme_tests_be_families io-cmp {device['uri']} --be {be_opts['be']} --alt-be {alt_be} --dev-nsid {device['nsid']}"
    )
    assert not err


@xnvme_parametrize(labels=["dev", "cuda"], opts=["be"])
def test_idfy_cmp(cijoe, device, be_opts, cli_args):
    alt_be = get_alt_be(be_opts["be"])
    if not alt_be:
        pytest.skip(reason=f"[be={be_opts['be']}] does not have an alternate backend")
    err, _ = cijoe.run(
        f"xnvme_tests_be_families idfy-cmp {device['uri']} --be {be_opts['be']} --alt-be {alt_be} --dev-nsid {device['nsid']}"
    )
    assert not err
