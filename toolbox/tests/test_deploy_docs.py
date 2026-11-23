"""
toolbox/ci/deploy-docs.sh against git repositories in tmp_path: a bare one
standing in for the GitHub Pages repository, a clone of it as the site, and a
stub of xnvme-docs-dest that places the documentation under en/<name of ref>
"""

import os
import subprocess
import tarfile
from pathlib import Path

import pytest

ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "toolbox" / "ci" / "deploy-docs.sh"

DEST = """#!/usr/bin/env bash
set -e
while [ $# -gt 0 ]; do
    case "$1" in
    --docs) docs=$2 ;;
    --site) site=$2 ;;
    --ref) ref=$2 ;;
    esac
    shift 2
done
mkdir -p "$site/en"
rm -rf "${site:?}/en/${ref##*/}"
cp -r "$docs" "$site/en/${ref##*/}"
"""

# Without the git configuration of the user, which may sign commits
ENV = {"GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1"}


def git(cwd, *args):
    return subprocess.run(
        ["git", "-c", "user.name=t", "-c", "user.email=t@t", *args],
        cwd=cwd,
        env=os.environ | ENV,
        check=True,
        capture_output=True,
        text=True,
    ).stdout


@pytest.fixture
def pages(tmp_path):
    """The bare repository, a clone of it, the site, and a docs.tar.gz"""
    remote = tmp_path / "remote.git"
    git(tmp_path, "init", "-q", "--bare", "-b", "main", str(remote))
    other = tmp_path / "other"
    git(tmp_path, "clone", "-q", str(remote), str(other))
    (other / "README.md").write_text("site\n")
    git(other, "add", ".")
    git(other, "commit", "-qm", "seed")
    git(other, "push", "-q", "origin", "HEAD:main")
    site = tmp_path / "site"
    git(tmp_path, "clone", "-q", str(remote), str(site))

    html = tmp_path / "build" / "html"
    html.mkdir(parents=True)
    (html / "index.html").write_text("docs of v1.2.3\n")
    tarball = tmp_path / "docs.tar.gz"
    with tarfile.open(tarball, "w:gz") as tar:
        tar.add(html, arcname="html/.")

    return remote, other, site, tarball


def deploy(tmp_path, tarball, site, ref):
    stubs = tmp_path / "stub-bin"
    stubs.mkdir(exist_ok=True)
    (stubs / "xnvme-docs-dest").write_text(DEST)
    (stubs / "sleep").write_text(f'#!/bin/sh\necho "$1" >> {tmp_path}/slept\n')
    for stub in stubs.iterdir():
        stub.chmod(0o755)

    return subprocess.run(
        ["bash", str(SCRIPT), str(tarball), str(site), ref],
        cwd=tmp_path,
        env=os.environ | ENV | {"PATH": f"{stubs}:{os.environ['PATH']}"},
        capture_output=True,
        text=True,
    )


def test_the_documentation_of_a_ref_is_pushed(tmp_path, pages):
    remote, _, site, tarball = pages

    run = deploy(tmp_path, tarball, site, "refs/tags/v1.2.3")

    assert run.returncode == 0, run.stderr
    assert git(remote, "show", "main:en/v1.2.3/index.html") == "docs of v1.2.3\n"
    assert git(remote, "log", "-1", "--format=%s", "main") == (
        "Auto-deployed update of 'docs/' for 'refs/tags/v1.2.3'\n"
    )


def test_a_push_that_the_other_deploy_got_in_first_is_tried_again(tmp_path, pages):
    """A release deploys main and its tag at once"""
    remote, other, site, tarball = pages
    (other / "en").mkdir()
    (other / "en" / "main").write_text("docs of main\n")
    git(other, "add", ".")
    git(other, "commit", "-qm", "main")
    git(other, "push", "-q", "origin", "HEAD:main")

    run = deploy(tmp_path, tarball, site, "refs/tags/v1.2.3")

    assert run.returncode == 0, run.stderr
    assert (tmp_path / "slept").read_text() == "10\n"
    assert git(remote, "show", "main:en/main") == "docs of main\n"
    assert git(remote, "show", "main:en/v1.2.3/index.html") == "docs of v1.2.3\n"


def test_documentation_the_site_has_is_not_committed_again(tmp_path, pages):
    remote, _, site, tarball = pages
    assert deploy(tmp_path, tarball, site, "refs/tags/v1.2.3").returncode == 0
    head = git(remote, "rev-parse", "main")

    run = deploy(tmp_path, tarball, site, "refs/tags/v1.2.3")

    assert run.returncode == 0, run.stderr
    assert "The site has these docs already" in run.stdout
    assert git(remote, "rev-parse", "main") == head
