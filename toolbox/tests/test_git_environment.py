import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def git(repo, *args):
    return subprocess.run(
        ["git", "-C", str(repo), *args], capture_output=True, text=True, check=True
    ).stdout.strip()


def test_tests_leave_the_repository_of_git_dir_alone(tmp_path):
    other = tmp_path / "other"
    subprocess.run(["git", "init", "-q", str(other)], check=True)

    run = subprocess.run(
        [
            sys.executable,
            "-m",
            "pytest",
            "-q",
            "-p",
            "no:cacheprovider",
            str(HERE / "test_xnvme_release.py"),
        ],
        env={**os.environ, "GIT_DIR": str(other / ".git")},
        capture_output=True,
        text=True,
    )

    assert run.returncode == 0, run.stdout
    assert git(other, "config", "core.bare") == "false"
    assert git(other, "rev-list", "--all") == ""
    assert git(other, "tag") == ""
