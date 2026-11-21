"""
Each test runs without the variables by which git selects a repository. In a
worktree, 'git rebase --exec' sets GIT_DIR, and the git commands that the tests
run in tmp_path then act on the repository of the checkout.
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
