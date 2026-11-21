"""
Tests run git in repositories they create in tmp_path. Each test runs without
the variables by which git selects a repository, such as GIT_DIR, which 'git
rebase --exec' sets in a worktree; with them, those git commands act on the
repository of the checkout, and 'git init' sets core.bare there.
"""

import subprocess

import pytest

LOCAL = subprocess.run(
    ["git", "rev-parse", "--local-env-vars"],
    capture_output=True,
    text=True,
    check=True,
).stdout.split()


@pytest.fixture(autouse=True)
def without_the_repository_of_the_environment(monkeypatch):
    for name in LOCAL:
        monkeypatch.delenv(name, raising=False)
