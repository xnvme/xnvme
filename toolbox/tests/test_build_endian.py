import subprocess
from pathlib import Path

ROOT = Path(__file__).parents[2]


def test_refuses_big_endian_host(tmp_path):
    cross = tmp_path / "s390x.ini"
    cross.write_text(
        "[binaries]\n"
        "c = 'gcc'\n"
        "\n"
        "[host_machine]\n"
        "system = 'linux'\n"
        "cpu_family = 's390x'\n"
        "cpu = 's390x'\n"
        "endian = 'big'\n"
    )

    proc = subprocess.run(
        ["meson", "setup", str(tmp_path / "builddir"), "--cross-file", str(cross)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert proc.returncode != 0
    assert "s390x is big-endian" in proc.stdout + proc.stderr
