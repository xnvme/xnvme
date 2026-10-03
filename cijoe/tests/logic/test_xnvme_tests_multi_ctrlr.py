import pytest

from ..conftest import cijoe_config_get_all_devices, xnvme_parametrize

# One of these says how a device is reached, and only a device reached the
# same way opens with the same backend
KINDS = ["bdev", "cdev", "pcie", "fabrics"]


def run_two(cijoe, device, be_opts, cli_args, labels):
    """
    Drive 'device' and a second controller of the same kind

    The other device is picked among those the configuration reaches the same
    way, so it opens with the backend under test, and among those holding a
    plain namespace, since the test writes and reads LBA 0. The same
    controller under another name is excluded by the kind: a block device, a
    character device and a PCIe address are three names for one controller.
    """

    kind = [label for label in KINDS if label in device["labels"]]
    if not kind or "ramdisk" in device["labels"]:
        pytest.skip(reason=f"No second controller of the kind: {device['labels']}")

    others = [
        candidate
        for candidate in cijoe_config_get_all_devices(labels + kind + ["nvm"])
        if candidate["uri"] != device["uri"] and "ramdisk" not in candidate["labels"]
    ]
    if not others:
        pytest.skip(reason=f"The configuration has only one device labelled: {labels}")

    # cli_args carries the HOMI id where the suite is served; without it the
    # program opens the controller underneath the server holding it.
    err, _ = cijoe.run(
        f"xnvme_tests_multi_ctrlr io {cli_args} --alt-uri {others[0]['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["dev", "nvm"], opts=["be"])
def test_io(cijoe, device, be_opts, cli_args):
    """Two controllers, one process, one buffer pool."""

    run_two(cijoe, device, be_opts, cli_args, ["dev"])


@xnvme_parametrize(labels=["dev", "nvm", "cuda"], opts=["be"])
def test_io_cuda(cijoe, device, be_opts, cli_args):
    """The same, with the buffers in device memory the GPU owns."""

    run_two(cijoe, device, be_opts, cli_args, ["dev", "cuda"])
