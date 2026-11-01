import importlib.util
import re
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "toolbox" / "xnvme_release.py"
spec = importlib.util.spec_from_file_location("xnvme_release", SCRIPT)
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)

CHANGELOG = """Changelog
=========

v1.2.3
------

* The notes of v1.2.3

  - with a sub-item

v1.2.2
------

* Older notes
"""


def bump(root, version):
    """Set every file carrying the version under 'root' to 'version'"""

    for path, pattern in release.VERSIONED:
        text = (root / path).read_text()
        current = re.search(pattern, text, re.M).group(1)
        (root / path).write_text(text.replace(current, version))


@pytest.fixture
def tree(tmp_path):
    """This tree's files carrying the version, at 1.2.3, and CHANGELOG"""

    for path, _ in release.VERSIONED:
        (tmp_path / path).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(ROOT / path, tmp_path / path)
    bump(tmp_path, "1.2.3")
    (tmp_path / "CHANGELOG.rst").write_text(CHANGELOG)

    return tmp_path


def test_bump_gives_its_section(tree):
    version, notes = release.check("ver: bump to v1.2.3", tree)
    assert version == "1.2.3"
    assert notes == "* The notes of v1.2.3\n\n  - with a sub-item"


def test_the_last_section_ends_with_the_file():
    assert release.notes_for(CHANGELOG, "1.2.2") == "* Older notes"


def test_not_a_bump(tree):
    with pytest.raises(ValueError, match="not a version bump"):
        release.check("CHANGELOG: update for v1.2.3", tree)


def test_a_file_not_bumped(tree):
    cargo = tree / "rust/xnvme-sys/Cargo.toml"
    cargo.write_text(cargo.read_text().replace('xnvme = "1.2.3"', 'xnvme = "1.2.2"'))
    with pytest.raises(ValueError, match="Cargo.toml has 1.2.2"):
        release.check("ver: bump to v1.2.3", tree)


def test_no_changelog_section(tree):
    bump(tree, "1.2.4")
    with pytest.raises(ValueError, match="no 'v1.2.4' section"):
        release.check("ver: bump to v1.2.4", tree)


def test_bump_sets_every_file(tree):
    release.bump("1.3.0", tree)
    (tree / "CHANGELOG.rst").write_text(CHANGELOG.replace("v1.2.3", "v1.3.0"))
    version, _ = release.check("ver: bump to v1.3.0", tree)
    assert version == "1.3.0"


def test_bump_writes_nothing_when_a_file_changed_shape(tree):
    cargo = tree / "rust/xnvme-sys/Cargo.toml"
    cargo.write_text(
        cargo.read_text().replace('xnvme = "1.2.3"', 'xnvme = { "1.2.3" }')
    )
    before = {path: (tree / path).read_text() for path, _ in release.VERSIONED}
    with pytest.raises(ValueError, match="no match"):
        release.bump("1.3.0", tree)
    assert before == {path: (tree / path).read_text() for path, _ in release.VERSIONED}


def test_bump_refuses_what_is_not_a_version(tree):
    with pytest.raises(ValueError, match="not a version"):
        release.bump("v1.3.0", tree)


def test_contributors_names_who_is_missing(tmp_path):
    """Only authors since the release tag count, and other tags are no release"""

    def git(*args):
        subprocess.run(
            ["git", "-C", str(tmp_path), "-c", "commit.gpgsign=false", *args],
            check=True,
            capture_output=True,
        )

    def commit(name):
        git(
            "-c",
            f"user.name={name}",
            "-c",
            "user.email=a@b.c",
            "commit",
            "-qm",
            name,
            "--allow-empty",
        )

    git("init", "-q")
    commit("Old Unlisted")
    git("tag", "v1.0.0")
    commit("Ada Listed")
    commit("Bo Missing")
    git("tag", "backup/x")
    commit("Ada Listed")
    (tmp_path / "CONTRIBUTORS.md").write_text("* Ada Listed <a@b.c>\n")

    assert release.missing_contributors(tmp_path) == ["Bo Missing"]


def test_contributors_without_a_release_tag(tmp_path):
    subprocess.run(["git", "-C", str(tmp_path), "init", "-q"], check=True)
    (tmp_path / "CONTRIBUTORS.md").write_text("")
    with pytest.raises(ValueError):
        release.missing_contributors(tmp_path)
