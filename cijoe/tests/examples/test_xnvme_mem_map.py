import pytest

from ..conftest import get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_cuda_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-cuda":
        pytest.skip(reason="The example opens the device with --be upcie-cuda")

    homi_id = get_homi_id()
    err, _ = cijoe.run(
        f"xnvme_cuda_mem_map {device['uri']} {homi_id if homi_id else ''}"
    )
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_hip_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-hip":
        pytest.skip(reason="The example opens the device with --be upcie-hip")

    homi_id = get_homi_id()
    err, _ = cijoe.run(
        f"xnvme_hip_mem_map {device['uri']} {homi_id if homi_id else ''}"
    )
    assert not err
