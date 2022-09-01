"""
    This is the test-configuration for xNVMe

    xnvme_be_opts(): helper-function used by xnvme_setup()

    xnvme_setup(): generates a list of sensible backend configurations and matches them
    up with devices from the CIJOE configuration. Emitting (device, be_opts, cli_args)
    and pytest.skip when a valid device is not found in the configuration.

    XnvmeDriver: provides a "functor" for controlling NVMe driver attachment.

    xnvme_driver: a fixture for parametrize, invoking XnvmeDriver as needed by the
    specific testcase, e.g. specific to the parametrization. This relies on the pytest
    feature "indirect parametrization".
"""
from functools import wraps

import pytest

from .xnvme_be_combinations import get_combinations


def xnvme_be_opts(options=None, only_labels=[]):
    """Produce a list of "sensible" backend configurations"""

    osname = pytest.cijoe_instance.config.options.get("os", {}).get("name", "linux")
    if osname == "debian":
        osname = "linux"

    if options is None:
        options = ["be", "mem", "sync", "async", "admin", "label"]
    if "label" not in options:
        options.append("label")

    combinations = get_combinations()

    all_configs = []
    for opts in combinations[osname]:
        for be in opts["be"]:
            for be_mem in opts["mem"]:
                for be_admin in opts["admin"]:
                    for be_sync in opts["sync"]:
                        for be_async in opts["async"]:
                            for label in opts["label"]:
                                if only_labels and label not in only_labels:
                                    continue

                                all_configs.append(
                                    {
                                        "be": be,
                                        "mem": be_mem,
                                        "admin": be_admin,
                                        "sync": be_sync,
                                        "async": be_async,
                                        "label": label,
                                    }
                                )

    filtered = []
    for cfg in all_configs:
        item = [(key, val) for (key, val) in cfg.items() if key in options]
        if item not in filtered:
            filtered.append(item)
    filtered.sort()

    return [dict(item) for item in filtered]


def cijoe_config_get_device(labels):
    """Returns the 'device-dict' from 'devices' in 'cijoe_cfg' with the given 'label'"""

    for device in pytest.cijoe_instance.config.options.get("devices", []):
        if not (set(labels) - set(device["labels"])):
            return device

    return None


def xnvme_setup_device(labels):
    """Get a device, without a backend-configuration to match"""

    device = cijoe_config_get_device(labels)
    if device:
        return [device]

    return [
        pytest.param(
            device,
            marks=pytest.mark.skip(
                f"xnvme_setup_device(): Configuration has no device labelled: {labels}"
            ),
        )
    ]


def xnvme_cli_args(device, be_opts):
    """Construct cli-arguments for the given device and backend options"""

    args = []

    if device:
        args += [f"{device['uri']}"]
        args += [f"--dev-nsid {device['nsid']}"]

    if be_opts:
        args += [f"--{arg} {val}" for arg, val in be_opts.items() if arg != "label"]

    return " ".join(args)


def xnvme_setup(labels=[], opts=[]):
    """Produces a config, yields (device, be_opts, cli_args)"""

    parametrization = []

    combinations = xnvme_be_opts(
        opts, ["file"] if "file" in labels else ["bdev", "cdev", "pcie", "fabrics"]
    )

    for be_opts in combinations:
        search = labels + [be_opts["label"]]
        device = cijoe_config_get_device(search)

        dstr = device["uri"] if device else "None"
        bstr = ",".join([f"{k}={v}" for k, v in be_opts.items()])

        paramid = f"uri={dstr},{bstr}"

        cli_args = xnvme_cli_args(device, be_opts)

        if device is None:
            parametrization.append(
                pytest.param(
                    device,
                    be_opts,
                    cli_args,
                    marks=pytest.mark.skip(
                        f"Configuration has no device labelled: {search}"
                    ),
                    id=paramid,
                )
            )
        else:
            parametrization.append(
                pytest.param(
                    device,
                    be_opts,
                    cli_args,
                    id=paramid,
                )
            )

    return parametrization


class XnvmeDriver(object):
    """
    The driver managing for an NVMe device is in Linux attached to one of:

    * nvme (Kernel managed NVMe driver)
    * vfio-pci (User space)
    * uio-generic (User space)

    Switching attachment is done via the 'xnvme-driver' cli-tool.

    This class encapsulates the cli-tool via static helpers (kernel_detach,
    kernal_attach). Whether or not a device needs one or the other is handled
    explicitly by the key: "driver_attachment" on the device in the configuration.

        driver_attachment: "kernel"

    or

        driver_attachment: "userspace"

    When no "driver_attachment" key is provided, "kernel" is assumed.

    Additionally, since switching device-driver-attachment takes a non-trivial amount of
    time. Especially when doing it repeatedly, as done when executing the testrunner.

    To reduce that processing time, then the XnvmeDriver.prep() function checks whether
    it needs to call the cli-tool tool, by examining XnvmeDriver.ATTACHED.
    """

    IS_KERNEL_ATTACHED = None

    @staticmethod
    def kernel_detach(cijoe):
        """Detach from kernel"""

        cijoe.run("xnvme-driver")
        XnvmeDriver.IS_KERNEL_ATTACHED = False

    @staticmethod
    def kernel_attach(cijoe):
        """Attach to kernel"""

        cijoe.run("xnvme-driver reset")
        XnvmeDriver.IS_KERNEL_ATTACHED = True

    @staticmethod
    def attach(cijoe, device):
        """Attach device driver according to the 'needs' of the given 'device'"""

        needs_kernel = device.get("driver_attachment", "kernel") == "kernel"
        needs_userspace = not needs_kernel

        if needs_userspace and XnvmeDriver.IS_KERNEL_ATTACHED in [True, None]:
            XnvmeDriver.kernel_detach(cijoe)
            cijoe.run("xnvme enum")
        elif needs_kernel and XnvmeDriver.IS_KERNEL_ATTACHED in [False, None]:
            XnvmeDriver.kernel_attach(cijoe)
            cijoe.run("xnvme enum")
        else:
            cijoe.run('echo "Skipping XnvmeDriver.attach()."')


@pytest.fixture
def device(cijoe, request):
    """
    This handles NVMe-device-driver-attachment per parametrized testcase.

    Usage example:

      from cijoe.xnvme.tests.config import xnvme_device_driver as device

      pytest.mark.parametrize("device,be_opts", {data}, indirect=["device"])
      test_foo(cijoe, device, be_opts):
        ...

    By doing so, then each generated parameter for the test-case is sent by this
    fixture. This allows the fixture to invoke device-driver attachment according to
    the given device. This avoids having to call:

      XnvmeDriver.attach(cijoe, request.param)

    from within the testcase body itself.
    """

    XnvmeDriver.attach(cijoe, request.param)

    return request.param


def xnvme_parametrize(labels, opts):
    """
    This decorator provides all the pytest-parametrization magic in one.

    Instead of decorating tests with, e.g.:

        @pytest.mark.parametrize(
            "device,be_opts,cli_args",
            xnvme_setup(labels=["bdev"], opts=["be", "admin"]),
            indirect=["device"],
        )

    Then they can be decorated with, e.g.:

        @xnvme_parametrize(labels=["bdev"]), opts=["be", "admin"])

    This encapsulates all the pytest-voodoo into the conftest
    """

    def decorator(fun):
        @pytest.mark.parametrize(
            "device,be_opts,cli_args",
            xnvme_setup(labels=labels, opts=opts),
            indirect=["device"],
        )
        @wraps(fun)
        def inner(*args, **kwargs):
            return fun(*args, **kwargs)

        return inner

    return decorator
