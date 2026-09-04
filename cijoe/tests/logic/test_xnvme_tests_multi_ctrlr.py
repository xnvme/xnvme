import pytest

from ..conftest import cijoe_config_get_all_devices, xnvme_parametrize


def run_two(cijoe, device, be_opts, labels):
    others = [
        candidate
        for candidate in cijoe_config_get_all_devices(labels)
        if candidate["uri"] != device["uri"]
    ]
    if not others:
        pytest.skip(reason=f"The configuration has only one device labelled: {labels}")

    err, _ = cijoe.run(
        f"xnvme_tests_multi_ctrlr io {device['uri']}"
        f" --alt-uri {others[0]['uri']}"
        f" --be {be_opts['be']} --dev-nsid {device['nsid']}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_io(cijoe, device, be_opts, cli_args):
    """Two controllers, one process, one buffer pool."""

    run_two(cijoe, device, be_opts, ["dev"])


@xnvme_parametrize(labels=["dev", "cuda"], opts=["be"])
def test_io_cuda(cijoe, device, be_opts, cli_args):
    """The same, with the buffers in device memory the GPU owns."""

    run_two(cijoe, device, be_opts, ["dev", "cuda"])
