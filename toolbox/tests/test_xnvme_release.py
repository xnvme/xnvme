import importlib.util
import io
import re
import shutil
import subprocess
import sys
import tarfile
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


def archive(
    tmp_path, tree, top="xnvme-1.2.3", extra=(), leave_out=(), vcs_tag=b"0123abc\n"
):
    """A source archive of 'tree' under the directory 'top', as meson dist
    writes it, with 'vcs_tag' in .vcs_tag and the subproject upcie; 'extra' are
    tarfile members, with their content, to add, and 'leave_out' the paths of
    files to leave out"""

    files = {
        path: (tree / path).read_bytes()
        for path in sorted({p for p, _ in release.VERSIONED} | {"CHANGELOG.rst"})
    }
    files[".vcs_tag"] = vcs_tag
    files["subprojects/upcie/upcie.h"] = b"/* upcie */\n"
    path = tmp_path / "xnvme-src.tar.gz"
    with tarfile.open(path, "w:gz") as tar:
        for name, content in files.items():
            if name in leave_out:
                continue
            info = tarfile.TarInfo(f"{top}/{name}")
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))
        for info, content in extra:
            tar.addfile(info, io.BytesIO(content) if content is not None else None)

    return path


def member(name, content=b"", **attrs):
    info = tarfile.TarInfo(name)
    info.size = len(content)
    for key, value in attrs.items():
        setattr(info, key, value)
    return info, content


def link(name, target):
    return member(name, type=tarfile.SYMTYPE, linkname=target)


def test_an_archive_of_the_tree_has_no_problem(tree, tmp_path):
    path = archive(tmp_path, tree, extra=[link("xnvme-1.2.3/COPY", "CHANGELOG.rst")])

    assert release.archive_problems(path, "1.2.3", ["upcie"]) == []


def test_git_metadata_in_an_archive_is_a_problem(tree, tmp_path):
    gitlink = member("xnvme-1.2.3/subprojects/spdk/dpdk/.git", b"gitdir: ../x\n")
    path = archive(tmp_path, tree, extra=[gitlink])

    assert release.archive_problems(path, "1.2.3") == [
        "xnvme-1.2.3/subprojects/spdk/dpdk/.git: git metadata"
    ]


def test_a_member_outside_the_directory_of_the_version_is_a_problem(tree, tmp_path):
    outside = [member("xnvme-1.2.2/README.md"), member("xnvme-1.2.3/../x")]
    path = archive(tmp_path, tree, extra=outside)

    assert release.archive_problems(path, "1.2.3") == [
        "xnvme-1.2.2/README.md: not under xnvme-1.2.3/",
        "xnvme-1.2.3/../x: not under xnvme-1.2.3/",
    ]


def test_a_link_out_of_the_archive_or_to_nothing_is_a_problem(tree, tmp_path):
    links = [
        link("xnvme-1.2.3/up", "../../etc/passwd"),
        link("xnvme-1.2.3/abs", "/etc/passwd"),
        link("xnvme-1.2.3/gone", "missing.txt"),
        member("xnvme-1.2.3/hard", type=tarfile.LNKTYPE, linkname="etc/passwd"),
        member("xnvme-1.2.3/hardgone", type=tarfile.LNKTYPE, linkname="xnvme-1.2.3/no"),
    ]
    path = archive(tmp_path, tree, extra=links)

    assert release.archive_problems(path, "1.2.3") == [
        "xnvme-1.2.3/up: a link to ../../etc/passwd, outside xnvme-1.2.3/",
        "xnvme-1.2.3/abs: a link to /etc/passwd, outside xnvme-1.2.3/",
        "xnvme-1.2.3/gone: a link to missing.txt, which the archive does not hold",
        "xnvme-1.2.3/hard: a link to etc/passwd, outside xnvme-1.2.3/",
        "xnvme-1.2.3/hardgone: a link to xnvme-1.2.3/no, which the archive does not hold",
    ]


def test_an_archive_without_its_revision_or_a_subproject_has_problems(tree, tmp_path):
    # A directory that holds only directories is no subproject
    empty = [
        member("xnvme-1.2.3/subprojects/spdk", type=tarfile.DIRTYPE),
        member("xnvme-1.2.3/subprojects/spdk/lib", type=tarfile.DIRTYPE),
    ]
    path = archive(tmp_path, tree, leave_out=[".vcs_tag"], extra=empty)

    assert release.archive_problems(path, "1.2.3", ["upcie", "spdk"]) == [
        "xnvme-1.2.3/.vcs_tag: missing, so a build of the archive reports no revision",
        "xnvme-1.2.3/subprojects/spdk/: missing or without files",
    ]


def test_the_archive_of_a_release_records_the_tag_of_the_release(tree, tmp_path):
    tagged = archive(tmp_path, tree, vcs_tag=b"v1.2.3\n")

    assert release.archive_problems(tagged, "1.2.3", ["upcie"], release=True) == []

    # A revision, the tag of a dirty tree, and the tag of another version
    for stamp in ("0123abc", "v1.2.3+", "v1.2.30"):
        other = archive(tmp_path, tree, vcs_tag=f"{stamp}\n".encode())

        assert release.archive_problems(other, "1.2.3", ["upcie"]) == []
        assert release.archive_problems(other, "1.2.3", ["upcie"], release=True) == [
            f"xnvme-1.2.3/.vcs_tag: {stamp}, not the tag v1.2.3 of the release"
        ]


def test_an_archive_of_another_version_is_a_problem(tree, tmp_path):
    bump(tree, "1.2.2")
    path = archive(tmp_path, tree)

    problems = release.archive_problems(path, "1.2.3")

    assert len(problems) == 1
    assert problems[0].startswith(
        "the commit bumps to 1.2.3, but meson.build has 1.2.2"
    )


def test_an_archive_without_a_file_carrying_the_version_is_a_problem(tree, tmp_path):
    path = archive(tmp_path, tree, leave_out=["cijoe/pyproject.toml"])

    assert release.archive_problems(path, "1.2.3") == [
        "xnvme-1.2.3/cijoe/pyproject.toml: missing"
    ]


def test_archive_checks_against_the_version_of_the_tree(tree, tmp_path):
    path = archive(tmp_path, tree, top="xnvme-1.2.2")
    run = subprocess.run(
        [sys.executable, str(SCRIPT), "--archive", str(path), "--root", str(tree)],
        capture_output=True,
        text=True,
    )

    assert run.returncode == 1
    assert "not under xnvme-1.2.3/" in run.stdout


def test_archive_checks_the_subprojects_it_is_given(tree, tmp_path):
    path = archive(tmp_path, tree)
    command = [sys.executable, str(SCRIPT), "--archive", str(path)]
    command += ["--root", str(tree), "--subprojects"]

    passed = subprocess.run(command + ["upcie"], capture_output=True, text=True)
    failed = subprocess.run(command + ["upcie,spdk"], capture_output=True, text=True)

    assert passed.returncode == 0, passed.stdout
    assert failed.returncode == 1
    assert failed.stdout.splitlines() == [
        "xnvme-1.2.3/subprojects/spdk/: missing or without files"
    ]


def test_archive_checks_the_tag_of_a_release(tree, tmp_path):
    path = archive(tmp_path, tree)
    run = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--archive",
            str(path),
            "--release",
            "--root",
            str(tree),
        ],
        capture_output=True,
        text=True,
    )

    assert run.returncode == 1
    assert "0123abc, not the tag v1.2.3 of the release" in run.stdout


@pytest.mark.parametrize("unreadable", ["missing", "not an archive", "truncated"])
def test_archive_reports_an_archive_it_cannot_read(tree, tmp_path, unreadable):
    path = tmp_path / "xnvme-src.tar.gz"
    if unreadable == "not an archive":
        path.write_text("xnvme\n")
    elif unreadable == "truncated":
        whole = archive(tmp_path, tree).read_bytes()
        path.write_bytes(whole[: len(whole) // 2])
    run = subprocess.run(
        [sys.executable, str(SCRIPT), "--archive", str(path), "--root", str(tree)],
        capture_output=True,
        text=True,
    )

    assert run.returncode == 1
    assert run.stderr.startswith(f"release: {path}: ")


def test_archive_reports_a_root_without_meson_build(tree, tmp_path):
    run = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--archive",
            str(archive(tmp_path, tree)),
            "--root",
            str(tmp_path / "nothing"),
        ],
        capture_output=True,
        text=True,
    )

    assert run.returncode == 1
    assert run.stderr.startswith("release: ")
    assert "meson.build" in run.stderr
