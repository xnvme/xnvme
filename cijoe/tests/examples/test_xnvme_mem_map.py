import pytest

from ..conftest import get_shm_id, xnvme_parametrize


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_cuda_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-cuda":
        pytest.skip(reason="The example opens the device with --be upcie-cuda")
    if get_shm_id():
        pytest.skip(reason="xnvme_cuda_mem_map does not take --shm_id as argument")

    err, _ = cijoe.run(f"xnvme_cuda_mem_map {device['uri']}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_hip_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-hip":
        pytest.skip(reason="The example opens the device with --be upcie-hip")
    if get_shm_id():
        pytest.skip(reason="xnvme_hip_mem_map does not take --shm_id as argument")

    err, _ = cijoe.run(f"xnvme_hip_mem_map {device['uri']}")
    assert not err
