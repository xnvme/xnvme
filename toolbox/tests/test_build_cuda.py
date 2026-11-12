import os
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).parents[2]
OPTS = ["-Dwith-spdk=disabled", "-Dwith-libvfn=disabled", "-Dwith-hip=disabled"]

pytestmark = pytest.mark.skipif(
    shutil.which("nvcc") is not None, reason="needs a host without nvcc"
)


@pytest.fixture
def cuda_runtime(tmp_path):
    """A stub CUDA toolkit, laid out as meson looks for one, with the runtime but no nvcc"""

    stub = tmp_path / "cuda"
    target = stub / "targets" / "x86_64-linux"
    (target / "include").mkdir(parents=True)
    (target / "lib").mkdir()
    (stub / "version.json").write_text(
        '{"cuda": {"version": "13.3.0"}, "cuda_cudart": {"version": "13.3.0"}}'
    )
    (target / "include" / "cuda_runtime_api.h").write_text(
        "#define CUDART_VERSION 13030\n"
    )
    (target / "include" / "cuda.h").write_text("#define CUDA_VERSION 13030\n")

    src, obj = tmp_path / "stub.c", tmp_path / "stub.o"
    src.write_text("int cuda_stub(void) { return 0; }\n")
    subprocess.run(["cc", "-c", "-fPIC", str(src), "-o", str(obj)], check=True)
    subprocess.run(
        ["ar", "rcs", str(target / "lib" / "libcudart_static.a"), str(obj)], check=True
    )
    subprocess.run(
        ["cc", "-shared", str(obj), "-o", str(target / "lib" / "libcuda.so")],
        check=True,
    )

    return stub


def setup(builddir, stub, with_cuda):
    return subprocess.run(
        ["meson", "setup", str(builddir), f"-Dwith-cuda={with_cuda}", *OPTS],
        cwd=ROOT,
        env={**os.environ, "CUDA_PATH": str(stub)},
        capture_output=True,
        text=True,
    )


def test_auto_disables_upcie_cuda_without_nvcc(cuda_runtime, tmp_path):
    proc = setup(tmp_path / "builddir", cuda_runtime, "auto")
    assert proc.returncode == 0, proc.stdout + proc.stderr

    config = (tmp_path / "builddir" / "xnvme_config.h").read_text()
    assert "#define XNVME_CUDA_ENABLED" in config
    assert "#undef XNVME_BE_UPCIE_CUDA_ENABLED" in config


def test_enabled_refuses_without_nvcc(cuda_runtime, tmp_path):
    proc = setup(tmp_path / "builddir", cuda_runtime, "enabled")
    assert proc.returncode != 0
    assert "nvcc" in proc.stdout + proc.stderr
