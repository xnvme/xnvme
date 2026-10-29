import pytest

from ..conftest import xnvme_parametrize


def get_alt_be(be: str, device):
    """The backend to compare 'be' with on 'device': upcie and its GPU backend"""
    if be == "upcie":
        return "upcie-hip" if "hip" in device["labels"] else "upcie-cuda"
    elif be in ("upcie-cuda", "upcie-hip"):
        return "upcie"
    else:
        return None


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open_dev_twice(cijoe, device, be_opts, cli_args):
    # We are repurposing "xnvme_tests_dual_be idfy-cmp" to verify that opening the same
    # device twice with the same backend works.
    err, _ = cijoe.run(
        f"xnvme_tests_dual_be idfy-cmp {device['uri']} --be {be_opts['be']} --alt-be {be_opts['be']} --dev-nsid {device['nsid']}"
    )
    assert not err


def dual_be(cijoe, device, be_opts, cmd):
    """Run 'xnvme_tests_dual_be <cmd>' with the backend and its alternate"""
    alt_be = get_alt_be(be_opts["be"], device)
    if not alt_be:
        pytest.skip(reason=f"[be={be_opts['be']}] does not have an alternate backend")
    err, _ = cijoe.run(
        f"xnvme_tests_dual_be {cmd} {device['uri']} --be {be_opts['be']} --alt-be {alt_be} --dev-nsid {device['nsid']}"
    )
    assert not err


@xnvme_parametrize(labels=["dev", "cuda"], opts=["be"])
def test_io_cmp(cijoe, device, be_opts, cli_args):
    dual_be(cijoe, device, be_opts, "io-cmp")


@xnvme_parametrize(labels=["dev", "cuda"], opts=["be"])
def test_idfy_cmp(cijoe, device, be_opts, cli_args):
    dual_be(cijoe, device, be_opts, "idfy-cmp")


@xnvme_parametrize(labels=["dev", "hip"], opts=["be"])
def test_io_cmp_hip(cijoe, device, be_opts, cli_args):
    dual_be(cijoe, device, be_opts, "io-cmp")


@xnvme_parametrize(labels=["dev", "hip"], opts=["be"])
def test_idfy_cmp_hip(cijoe, device, be_opts, cli_args):
    dual_be(cijoe, device, be_opts, "idfy-cmp")
