import subprocess
from pathlib import Path

ROOT = Path(__file__).parents[2]


def setup(tmp_path, *opts):
    cross = tmp_path / "riscv64.ini"
    cross.write_text(
        "[binaries]\n"
        "c = 'gcc'\n"
        "\n"
        "[host_machine]\n"
        "system = 'linux'\n"
        "cpu_family = 'riscv64'\n"
        "cpu = 'riscv64'\n"
        "endian = 'little'\n"
    )

    return subprocess.run(
        [
            "meson",
            "setup",
            str(tmp_path / "builddir"),
            "--cross-file",
            str(cross),
            *opts,
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )


def test_auto_disables_upcie_without_barriers(tmp_path):
    proc = setup(tmp_path)
    assert proc.returncode == 0, proc.stdout + proc.stderr

    config = (tmp_path / "builddir" / "xnvme_config.h").read_text()
    assert "#undef XNVME_BE_UPCIE_ENABLED" in config


def test_enabled_refuses_without_barriers(tmp_path):
    proc = setup(tmp_path, "-Dwith-upcie=enabled")
    assert proc.returncode != 0
    assert "uPCIe has memory barriers only for x86 and aarch64" in (
        proc.stdout + proc.stderr
    )
