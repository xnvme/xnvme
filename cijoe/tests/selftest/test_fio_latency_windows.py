import importlib.util
import subprocess
from pathlib import Path

SCRIPT = Path(__file__).parents[2] / "scripts" / "fio_latency.py"
spec = importlib.util.spec_from_file_location("fio_latency", SCRIPT)
fio_latency = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fio_latency)

DRIVE = fio_latency.Device("bdev", 1, "//./PhysicalDrive1", ["bdev"], "kernel")


def filename_given_to_fio(engine, definition):
    """The --filename fio gets, once bash has run the command on Windows"""

    engine = fio_latency.determine_engine(engine, definition, [DRIVE], None)
    fio = fio_latency.FIO(
        engine,
        "fio",
        [DRIVE],
        "job",
        "randread",
        "1G",
        "4096",
        "json",
        "1",
        "1",
        "10",
        "5",
        "1",
        "1",
    )
    cmd = " ".join(fio.cmd("windows")).replace("fio", "printf '%s\\n'", 1)
    args = subprocess.run(["bash", "-c", cmd], capture_output=True, text=True)
    return next(a for a in args.stdout.splitlines() if a.startswith("--filename="))


def test_windowsaio_gets_the_device_namespace():
    definition = {"device": "bdev", "group": "windowsaio"}
    filename = filename_given_to_fio("windowsaio", definition)
    assert filename == "--filename=\\\\.\\PhysicalDrive1"


def test_xnvme_keeps_the_forward_slashes():
    definition = {"device": "bdev", "group": "windowsaio", "be": "iocp"}
    filename = filename_given_to_fio("xnvme_iocp", definition)
    assert filename == "--filename=//./PhysicalDrive1"
