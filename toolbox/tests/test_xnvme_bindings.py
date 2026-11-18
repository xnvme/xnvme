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


def functions_of_the_ir(tmp_path):
    """The functions of yace's IR of the C API, as the bindings are generated
    from it, by name"""

    pytest.importorskip("yace")
    yaml = pytest.importorskip("yaml")

    (tmp_path / "meta.yaml").write_text(bindings.meta("python"))
    cmd = [sys.executable, "-m", "yace", "--meta", str(tmp_path / "meta.yaml")]
    cmd += ["-I", ".", "--lenient", "--output", str(tmp_path / "ir")]
    for name in bindings.PRELUDE:
        cmd += ["--prelude", name]
    cmd += ["--"] + bindings.headers()
    subprocess.run(cmd, cwd=bindings.INCLUDE, check=True, capture_output=True)

    loader = getattr(yaml, "CSafeLoader", yaml.SafeLoader)
    ir = yaml.load((tmp_path / "ir" / "xnvme.yaml").read_text(), Loader=loader)
    return {
        entity["name"]: entity
        for entity in ir["entities"]
        if entity.get("key") == "function_decl"
    }


def test_allocations_may_return_null_and_give_phys_as_a_result(tmp_path):
    """The allocations of libxnvme_buf.h return NULL on error, 'phys' of the
    phys allocations and of xnvme_buf_vtophys() is written by the function, and
    that of the allocations may be NULL"""

    funcs = functions_of_the_ir(tmp_path)

    for name in [
        "xnvme_buf_alloc",
        "xnvme_buf_realloc",
        "xnvme_buf_phys_alloc",
        "xnvme_buf_phys_realloc",
        "xnvme_buf_virt_alloc",
    ]:
        assert funcs[name].get("nullable"), name
    for name in ["xnvme_buf_phys_alloc", "xnvme_buf_phys_realloc", "xnvme_buf_vtophys"]:
        (phys,) = [p for p in funcs[name]["params"] if p["name"] == "phys"]
        assert phys.get("direction") == "out", name
        assert bool(phys.get("nullable")) == (name != "xnvme_buf_vtophys"), name


def test_payloads_and_completions_are_arrays_of_a_named_length(tmp_path):
    """The payloads of the pass functions are arrays of the length their
    '_nbytes' gives, which the device reads or writes and which may be NULL;
    'dvec' of xnvme_cmd_pass_iov() is left raw. 'completions' of
    xnvme_queue_reap() is an array of the length 'max' and the count written
    'ncompleted'"""

    funcs = functions_of_the_ir(tmp_path)

    def params(name):
        return {param["name"]: param for param in funcs[name]["params"]}

    payloads = {
        "xnvme_cmd_pass": ["dbuf", "mbuf"],
        "xnvme_cmd_pass_admin": ["dbuf", "mbuf"],
        "xnvme_cmd_pass_iov": ["mbuf"],
    }
    for name, names in payloads.items():
        for buf in names:
            param = params(name)[buf]
            assert param.get("direction") == "inout", (name, buf)
            assert param.get("nullable"), (name, buf)
            assert param.get("length") == f"{buf}_nbytes", (name, buf)
    assert "length" not in params("xnvme_cmd_pass_iov")["dvec"]

    completions = params("xnvme_queue_reap")["completions"]
    assert completions.get("direction") == "out"
    assert completions.get("length") == "max"
    assert completions.get("count") == "ncompleted"
