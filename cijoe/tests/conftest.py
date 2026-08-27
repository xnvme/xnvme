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
from time import sleep

import pytest

from .xnvme_be_combinations import get_backend_configurations

_homi_id = 0


def pytest_addoption(parser):
    parser.addoption(
        "--homi-id",
        type=int,
        default=0,
        help="Open devices against the given control-plane id (0 = single process)",
    )


def pytest_configure(config):
    global _homi_id
    try:
        _homi_id = config.getoption("--homi-id", default=0)
    except ValueError:
        _homi_id = 0


def get_osname():
    """Return normalized OS name from CIJOE config"""
    osname = pytest.cijoe_instance.getconf("os.name", "linux")
    if osname == "debian":
        osname = "linux"
    return osname


def get_homi_id():
    return _homi_id if _homi_id else None


def xnvme_be_opts(options=None, only_labels=[]):
    """Produce a list of "sensible" backend configurations"""

    osname = get_osname()

    if options is None:
        options = ["be", "mem", "sync", "async", "admin", "label"]
    if "label" not in options:
        options.append("label")

    combinations = get_backend_configurations()

    all_configs = []
    for opts in combinations[osname]:
        # Skip combinations whose declared labels don't intersect only_labels.
        if only_labels and not any(lbl in only_labels for lbl in opts["label"]):
            continue
        for be in opts["be"]:
            for be_mem in opts["mem"]:
                for be_admin in opts["admin"]:
                    for be_sync in opts["sync"]:
                        for be_async in opts["async"]:
                            all_configs.append(
                                {
                                    "be": be,
                                    "mem": be_mem,
                                    "admin": be_admin,
                                    "sync": be_sync,
                                    "async": be_async,
                                    "label": opts["label"],
                                    "cplane": opts.get("cplane", False),
                                }
                            )

    filtered = []
    seen = []
    for cfg in all_configs:
        key = [(k, v) for k, v in cfg.items() if k in options]
        if key not in seen:
            seen.append(key)
            filtered.append(cfg)
    filtered.sort(key=lambda d: [(k, v) for k, v in d.items() if k in options])

    return filtered


def cijoe_config_get_all_devices(labels):
    """Returns the 'device-dict' from every device in 'cijoe_cfg' with the given 'label'"""
    devices = []
    for device in pytest.cijoe_instance.getconf("devices", []):
        if not (set(labels) - set(device["labels"])):
            devices.append(device)

    return devices


def cijoe_config_get_device(labels):
    """Returns the 'device-dict' from 'devices' in 'cijoe_cfg' with the given 'label'"""

    for device in pytest.cijoe_instance.getconf("devices", []):
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

    if _homi_id > 0:
        # One spelling for every backend: --homi-id fills both the field uPCIe
        # reads and the segment id SPDK hands DPDK.
        args += [f"--homi-id {_homi_id}"]

    return " ".join(args)


def xnvme_setup(labels=[], opts=[]):
    """Produces a config, yields (device, be_opts, cli_args)"""

    parametrization = []

    combinations = xnvme_be_opts(
        opts, ["file"] if "file" in labels else ["bdev", "cdev", "pcie", "fabrics"]
    )

    for be_opts in combinations:
        search = labels + be_opts["label"]
        device = cijoe_config_get_device(search)

        dstr = device["uri"] if device else "None"
        bstr = ",".join([f"{k}={v}" for k, v in be_opts.items() if k != "cplane"])

        paramid = f"uri={dstr},{bstr}"

        cli_opts = {k: v for k, v in be_opts.items() if k in opts}
        cli_args = xnvme_cli_args(device, cli_opts)

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

        # Rebinding devices and re-allocating hugepages invalidates a running server,
        # so tear it down first rather than leaving CPlaneServer with a stale handle.
        CPlaneServer.stop(cijoe)

        # 64MB contigmem buffers avoid ENOMEM on FreeBSD's fragmented CI heap;
        # see toolbox/xnvme-driver.sh.
        cmd = "xnvme-driver"
        if get_osname() == "freebsd":
            cmd = "env CONTIGMEM_BUFSZ=64 " + cmd

        err, _ = cijoe.run(cmd)
        if err:
            cijoe.run("dmesg | tail -n20")

        XnvmeDriver.IS_KERNEL_ATTACHED = False

    @staticmethod
    def kernel_attach(cijoe):
        """Attach to kernel"""

        CPlaneServer.stop(cijoe)

        err, _ = cijoe.run("xnvme-driver reset")
        if err:
            cijoe.run("dmesg | tail -n20")

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
    if request.param:
        if _homi_id:
            callspec = getattr(request.node, "callspec", None)
            be_opts = callspec.params.get("be_opts") if callspec else None
            if be_opts:
                if be_opts.get("cplane"):
                    CPlaneServer.start(cijoe, be_opts["be"], be_opts.get("label"))
                else:
                    pytest.skip(f"{be_opts.get('be')} cannot share a controller")
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


class CPlaneServer:
    """
    Manages the lifecycle of a homi server daemon.

    A single server holds every device carrying the labels of the backend-configuration
    under test, so it can serve whichever device a testcase asks for, as well as
    testcases enumerating and opening all of them. Labels vary per backend; the GPU
    backends use 'cuda'/'hip' where the host backend uses 'pcie'.

    Tracks the backend and labels the running server was started with (similar to
    XnvmeDriver.IS_KERNEL_ATTACHED), so it is only restarted when those change.
    """

    RUNNING = None
    CIJOE = None
    TIMEOUT = 60

    # Emitted by homi once every device it was given is open
    READY_MARKER = "HOMI started successfully"

    @staticmethod
    def is_running(cijoe):
        """Returns True when a homi server is running"""

        # Match on the process name; 'pgrep -f' also matched the shell that cijoe.run()
        # wraps the command in, so it never detected a failed start
        err, _ = cijoe.run("pgrep -x homi")

        return not err

    @staticmethod
    def is_ready(cijoe, be):
        """Returns True when the homi server has opened all of its devices"""

        err, _ = cijoe.run(
            f"grep -q '{CPlaneServer.READY_MARKER}' /tmp/cplane_{be}.out"
        )

        return not err

    @staticmethod
    def start(cijoe, be, labels=None):
        labels = labels if labels else ["pcie"]

        if CPlaneServer.RUNNING == (be, tuple(labels)):
            return

        devices = cijoe_config_get_all_devices(labels)
        uris = " ".join(d["uri"] for d in devices)
        if not uris:
            pytest.skip(f"Configuration has no device labelled: {labels}")

        XnvmeDriver.kernel_detach(cijoe)

        # A server nobody here started is holding the devices, which a run
        # before this one left behind. Every testcase wanting a server would
        # otherwise skip on the probe below, because the device it wants is
        # taken, and a suite reporting skips exits zero: a broken environment
        # would read as a run where nothing applied.
        if CPlaneServer.RUNNING is None and CPlaneServer.is_running(cijoe):
            cijoe.run("pgrep -a homi")
            pytest.fail(
                "a homi server is already running that this run did not start;"
                " it holds the devices, so nothing here can serve them. A run"
                " killed before its teardown leaves one behind: 'pkill -9 -x"
                " homi' clears it"
            )

        # A backend that cannot open the device at all on this machine is not a
        # defect in what is under test, and a server that fails for that reason
        # tells us nothing. Ask first, so that the environment produces a skip
        # while a server that cannot start over devices that do open produces a
        # failure.
        err, _ = cijoe.run(f"xnvme info {devices[0]['uri']} --be {be}")
        if err:
            pytest.skip(f"be({be}) cannot open {devices[0]['uri']} on this machine")

        # SPDK backs its DMA memory with files in the hugetlbfs mount and never unlinks
        # them, since clients must be able to map them. As xNVMe does not call
        # spdk_env_fini(), they outlive the process and keep holding hugepages. Drop them
        # so the server starting here does so with a full pool.
        mount_point = cijoe.getconf("hugetlbfs.mount_point", "/mnt/huge")
        cijoe.run(f"rm -f {mount_point}/spdk*map_*")

        # 'stdbuf -oL' keeps the readiness marker from sitting in libc's fully-buffered
        # stdout while the server parks waiting for a signal.
        #
        # 'setsid' and the closed stdin are what let this run against a remote
        # target: nohup alone leaves the server holding the transport's
        # channel, so the command never returns and the server is reported as
        # not running. Harmless where the target is localhost.
        cijoe.run(
            f"setsid stdbuf -oL homi start {uris} --be {be} --homi-id {_homi_id} "
            f"< /dev/null > /tmp/cplane_{be}.out 2>&1 &"
        )

        # Opening a device takes about a second, so waiting a fixed duration either
        # wastes time or lets testcases attach while the server is still opening the
        # rest, where they fail with the server unresponsive on the DPDK mp channel.
        for _ in range(CPlaneServer.TIMEOUT):
            if CPlaneServer.is_ready(cijoe, be):
                break
            if not CPlaneServer.is_running(cijoe):
                cijoe.run(f"cat /tmp/cplane_{be}.out")
                pytest.fail(f"homi server for be({be}) is not running")
            sleep(1)
        else:
            cijoe.run(f"cat /tmp/cplane_{be}.out")
            pytest.fail(f"homi server for be({be}) did not become ready")

        CPlaneServer.RUNNING = (be, tuple(labels))
        CPlaneServer.CIJOE = cijoe

    @staticmethod
    def forget():
        """
        Drop the record of a server that is already gone

        For a testcase that ended the server itself, where stop() has nothing
        left to ask for or wait on. The next testcase to want one starts it.
        """

        CPlaneServer.RUNNING = None
        CPlaneServer.CIJOE = None

    @staticmethod
    def stop(cijoe):
        if CPlaneServer.RUNNING is None:
            return

        CPlaneServer.RUNNING = None
        CPlaneServer.CIJOE = None

        cijoe.run("pkill -x homi || true")

        # Closing a controller is not instant, and a server still holding the device
        # keeps its successor from claiming it, so wait for it to actually be gone
        for _ in range(CPlaneServer.TIMEOUT):
            if not CPlaneServer.is_running(cijoe):
                break
            sleep(1)
        else:
            pytest.fail("homi server did not terminate")


@pytest.fixture(scope="session", autouse=True)
def cplane_teardown():
    yield
    if CPlaneServer.CIJOE is not None:
        CPlaneServer.stop(CPlaneServer.CIJOE)


# Sort order for pytest_collection_modifyitems. `be` first because backend
# switches are the expensive operation; the rest tie-break within a backend.
_BE_KEYS = ("be", "admin", "sync", "async", "mem")


def pytest_collection_modifyitems(config, items):
    """Group all variants for one backend together in the run timeline.

    Sort items by backend config first and test name second so the timeline
    runs as test_foo[kernel] -> test_bar[kernel] -> ... -> test_foo[spdk]
    -> test_bar[spdk] -> ..., minimising the number of driver-swap
    boundaries in the run and clustering failures by backend. Tests without
    parametrization sort before the parametrized batches with their
    relative order preserved.
    """

    def key(item):
        callspec = getattr(item, "callspec", None)
        if callspec is None:
            return ("",) * len(_BE_KEYS) + (item.name,)
        be_opts = callspec.params.get("be_opts")
        if not isinstance(be_opts, dict):
            return ("",) * len(_BE_KEYS) + (item.name,)
        return tuple(str(be_opts.get(k, "")) for k in _BE_KEYS) + (item.name,)

    items.sort(key=key)
