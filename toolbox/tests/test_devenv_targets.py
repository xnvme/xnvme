"""
The devenv-* make targets: work that needs no guest runs in the host container
without one, and devenv-up still boots one
"""

import subprocess
from pathlib import Path

ROOT = Path(__file__).parents[2]


def make(*args, dry_run=True):
    return subprocess.run(
        ["make", "-C", str(ROOT), "--no-print-directory"]
        + (["-n"] if dry_run else [])
        + ["DEVENV_ENGINE=false", "DEVENV_NAME=devenv-test", *args],
        capture_output=True,
        text=True,
    )


def test_devenv_run_runs_the_command_in_the_container_without_a_guest():
    proc = make("devenv-run", 'CMD=echo "a b" && make gen-man-pages')
    assert proc.returncode == 0, proc.stderr
    assert "false run -d --name devenv-test" in proc.stdout
    assert "false exec" in proc.stdout and "make gen-man-pages'" in proc.stdout
    assert "guest-" not in proc.stdout


def test_devenv_run_refuses_without_a_command():
    proc = make("devenv-run", dry_run=False)
    assert proc.returncode != 0
    assert "give CMD=<command>" in proc.stderr


def test_devenv_up_still_boots_a_guest_in_the_container():
    proc = make("devenv-up", "DEVENV_GUEST=debian-trixie-iommu_disabled")
    assert proc.returncode == 0, proc.stderr
    assert "false run -d --name devenv-test" in proc.stdout
    assert "make guest-start" in proc.stdout
