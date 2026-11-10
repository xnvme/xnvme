import importlib.util
import subprocess
import sys
from pathlib import Path

import pytest

SCRIPT = Path(__file__).parents[2] / "toolbox" / "xnvme_bindings.py"
module = importlib.util.spec_from_file_location("xnvme_bindings", SCRIPT)
bindings = importlib.util.module_from_spec(module)
module.loader.exec_module(bindings)


def test_headers_are_the_umbrellas_less_the_excluded():
    included = bindings.headers()

    assert included[0] == "libxnvme_util.h", "the umbrella's order is kept"
    assert "libxnvme_dev.h" in included
    assert not set(bindings.EXCLUDED) & set(included)


def test_every_excluded_header_is_in_the_umbrella():
    """A header renamed or dropped would otherwise leave its exclusion stale"""

    umbrella = (bindings.INCLUDE / bindings.UMBRELLA).read_text()
    for name in bindings.EXCLUDED:
        assert f'"{name}"' in umbrella, f"{name} is excluded but not included"


def test_every_header_exists():
    for name in bindings.headers():
        assert (bindings.INCLUDE / name).is_file(), name


def test_meta_has_the_license_of_each_package():
    assert "license: Apache-2.0" in bindings.meta("python")
    assert "license: BSD-3-Clause" in bindings.meta("rust")


def test_generates_the_python_module(tmp_path):
    pytest.importorskip("yace")

    path = tmp_path / "xnvme.py"
    assert not bindings.main(["python", str(path), "--no-check"])

    proc = subprocess.run(
        [sys.executable, "-c", "import xnvme; print(xnvme.XnvmeOpts.__name__)"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
    )
    assert proc.stdout.strip() == "XnvmeOpts", proc.stderr
