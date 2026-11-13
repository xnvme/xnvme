import pytest

from ..conftest import cijoe_config_get_device, get_homi_id, xnvme_parametrize


def mem_map_args(device, be_opts):
    """The device, the homi-id, and a second controller where one is configured"""

    homi_id = get_homi_id()
    args = [device["uri"], str(homi_id if homi_id else 0)]

    second = cijoe_config_get_device(be_opts["label"] + ["second_ctrlr"])
    if second:
        args.append(second["uri"])

    return " ".join(args)


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_cuda_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-cuda":
        pytest.skip(reason="The example opens the device with --be upcie-cuda")

    err, _ = cijoe.run(f"xnvme_cuda_mem_map {mem_map_args(device, be_opts)}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_hip_mem_map(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "upcie-hip":
        pytest.skip(reason="The example opens the device with --be upcie-hip")

    err, _ = cijoe.run(f"xnvme_hip_mem_map {mem_map_args(device, be_opts)}")
    assert not err
