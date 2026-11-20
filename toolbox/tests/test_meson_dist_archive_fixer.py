import subprocess
import sys
from pathlib import Path

SCRIPT = Path(__file__).parents[2] / "toolbox" / "meson_dist_archive_fixer.py"


def test_the_git_metadata_of_subprojects_and_their_submodules_is_removed(tmp_path):
    builddir = tmp_path / "builddir"
    spdk = builddir / "meson-dist" / "xnvme-1.2.3" / "subprojects" / "spdk"
    (spdk / ".git").mkdir(parents=True)
    (spdk / "dpdk").mkdir()
    (spdk / "dpdk" / ".git").write_text("gitdir: ../.git/modules/dpdk\n")
    (spdk / "dpdk" / "meson.build").write_text("project('dpdk')\n")

    subprocess.run([sys.executable, str(SCRIPT)], cwd=builddir, check=True)

    assert not (spdk / ".git").exists()
    assert not (spdk / "dpdk" / ".git").exists()
    assert (spdk / "dpdk" / "meson.build").exists()
