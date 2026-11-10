"""
The example of reading at a queue depth, xpy_perf, completes every read both
by callback and by reaping, on the ramdisk, which needs no device
"""

import importlib.machinery
import importlib.util
from pathlib import Path

import pytest

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
