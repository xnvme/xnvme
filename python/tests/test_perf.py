"""
The example of reading at a queue depth, xpy_perf, completes every read both
by callback and by reaping, on the ramdisk, which needs no device, and through
the module that --bindings names
"""

import importlib.machinery
import importlib.util
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
import xnvme

XPY_PERF = Path(__file__).resolve().parents[1] / "bindings" / "bin" / "xpy_perf"


def xpy_perf():
    """The program, as a module, from the source tree"""

    loader = importlib.machinery.SourceFileLoader("xpy_perf", str(XPY_PERF))
    module = importlib.util.module_from_spec(
        importlib.util.spec_from_loader("xpy_perf", loader)
    )
    loader.exec_module(module)
    return module


@pytest.mark.parametrize("completion", ["cb", "reap"])
def test_xpy_perf_completes_every_read(completion):
    perf = xpy_perf()
    args = perf.parse_args(
        [
            "--uri=1GB",
            "--be=ramdisk",
            "--async=nil",
            f"--completion={completion}",
            "--qdepth=8",
            "--ios=1000",
        ]
    )
    binding, _ = perf.load(None)
    result = perf.run(binding, args)

    assert result["ios"] == 1000
    assert result["errors"] == 0


def test_xpy_perf_needs_a_read():
    """The rate is per read completed, so --ios below 1 is a usage error"""

    with pytest.raises(SystemExit):
        xpy_perf().parse_args(["--uri=1GB", "--ios=0"])


def test_xpy_perf_reads_through_the_module_of_bindings(tmp_path):
    """--bindings loads the module xnvme in the directory given, here a copy of
    the installed module, in place of the installed one"""

    copy = tmp_path / "xnvme"
    shutil.copytree(
        Path(xnvme.__file__).parent,
        copy,
        ignore=shutil.ignore_patterns("__pycache__", "tests"),
    )

    # A process of its own, since this one has imported the installed module
    proc = subprocess.run(
        [
            sys.executable,
            str(XPY_PERF),
            f"--bindings={tmp_path}",
            "--uri=1GB",
            "--be=ramdisk",
            "--async=nil",
            "--qdepth=8",
            "--ios=1000",
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr

    result = dict(line.strip().split(": ", 1) for line in proc.stdout.splitlines()[1:])
    assert result["bindings"] == f"'{copy / '__init__.py'}'"
    assert result["ios"] == "1000"
