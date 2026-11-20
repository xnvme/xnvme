import os
import subprocess
import sys
from pathlib import Path

SCRIPT = Path(__file__).parents[2] / "toolbox" / "vcs_tag.py"


def git(repo, *args):
    return subprocess.run(
        [
            "git",
            "-C",
            str(repo),
            "-c",
            "user.name=Test",
            "-c",
            "user.email=test@example.com",
            *args,
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()


def repo_at(tmp_path, subject):
    """A repository whose one commit has the subject 'subject'"""

    repo = tmp_path / "repo"
    repo.mkdir(parents=True)
    git(repo, "init", "-q")
    (repo / "meson.build").write_text("project('xnvme')\n")
    git(repo, "add", "meson.build")
    git(repo, "commit", "-q", "-m", subject)

    return repo


def dist(repo, tmp_path, *args):
    """The .vcs_tag that 'vcs_tag.py --dist' writes for the commit of 'repo'"""

    root = tmp_path / "dist"
    root.mkdir(parents=True)
    subprocess.run(
        [sys.executable, str(SCRIPT), "--dist", *args],
        env=dict(os.environ, MESON_SOURCE_ROOT=str(repo), MESON_DIST_ROOT=str(root)),
        check=True,
    )

    return (root / ".vcs_tag").read_text()


def test_the_archive_of_a_bump_records_the_tag_of_the_release(tmp_path):
    repo = repo_at(tmp_path, "ver: bump to v1.2.3")

    assert dist(repo, tmp_path, "1.2.3") == "v1.2.3\n"


def test_the_archive_of_a_bump_with_changes_records_them_after_the_tag(tmp_path):
    repo = repo_at(tmp_path, "ver: bump to v1.2.3")
    (repo / "meson.build").write_text("project('xnvme', version: '1.2.3')\n")

    assert dist(repo, tmp_path, "1.2.3") == "v1.2.3+\n"


def test_the_archive_of_another_commit_records_its_revision(tmp_path):
    for i, subject in enumerate(["feat: add a thing", "ver: bump to v1.2.4"]):
        repo = repo_at(tmp_path / str(i), subject)
        revision = git(repo, "describe", "--tags", "--dirty=+", "--always")

        assert dist(repo, tmp_path / str(i), "1.2.3") == f"{revision}\n"


def test_dist_requires_the_version():
    run = subprocess.run(
        [sys.executable, str(SCRIPT), "--dist"], capture_output=True, text=True
    )

    assert run.returncode == 2
    assert "usage: vcs_tag.py --dist VERSION" in run.stderr


def test_meson_dist_passes_the_version_of_the_project():
    """vcs_tag.py --dist writes the tag only when the version it is given is
    that of the bump, so meson dist gives it the version meson.build carries"""

    meson = (Path(__file__).parents[2] / "meson.build").read_text()

    assert (
        "meson.add_dist_script(python3, vcs_tag_py, '--dist', meson.project_version())\n"
        in meson
    )
