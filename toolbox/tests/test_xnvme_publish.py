import hashlib
import importlib.util
import io
import json
import os
import subprocess
import sys
import tarfile
from pathlib import Path

import pytest

SCRIPT = Path(__file__).parents[2] / "toolbox" / "xnvme_publish.py"
spec = importlib.util.spec_from_file_location("xnvme_publish", SCRIPT)
publish = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publish)


def targz(path, files):
    """Write a tar.gz at 'path' of 'files', {name: content}"""

    with tarfile.open(path, "w:gz") as tar:
        for name, content in files.items():
            info = tarfile.TarInfo(name)
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))


def release(version, wrong=()):
    """A download() of the assets of the release 'version', as the publish
    workflow attaches them, with a wrong .sha256sum for the names in 'wrong'"""

    def download(version_, directory):
        assert version_ == version
        sdist, source = publish.assets(version)
        targz(directory / sdist, {f"xnvme-{version}/PKG-INFO": b"Name: xnvme\n"})
        targz(
            directory / source,
            {f"xnvme-{version}/rust/xnvme-sys/Cargo.toml": b"[package]\n"},
        )
        for name in (sdist, source):
            digest = hashlib.sha256((directory / name).read_bytes()).hexdigest()
            if name in wrong:
                digest = "0" * 64
            (directory / f"{name}.sha256sum").write_text(f"{digest}  {name}\n")

    return download


def test_fetch_gives_the_sdist_by_the_name_pypi_requires_and_the_crate(tmp_path):
    sdist, manifest = publish.fetch("1.2.3", tmp_path / "out", release("1.2.3"))

    assert sdist == tmp_path / "out" / "pypi" / "xnvme-1.2.3.tar.gz"
    with tarfile.open(sdist) as tar:
        assert tar.getnames() == ["xnvme-1.2.3/PKG-INFO"]
    assert manifest == (
        tmp_path / "out" / "src" / "xnvme-1.2.3" / "rust/xnvme-sys/Cargo.toml"
    )
    assert manifest.read_text() == "[package]\n"


@pytest.mark.parametrize("which", [0, 1])
def test_fetch_stops_on_an_asset_its_sha256sum_does_not_match(tmp_path, which):
    name = publish.assets("1.2.3")[which]

    with pytest.raises(ValueError, match=f"{name} has the sha256"):
        publish.fetch("1.2.3", tmp_path / "out", release("1.2.3", wrong=[name]))
    assert not (tmp_path / "out" / "src").exists()
    assert not (tmp_path / "out" / "pypi").exists()


def test_crates_io_has_a_version_its_index_lists():
    index = "\n".join(
        json.dumps({"name": "xnvme-sys", "vers": vers}) for vers in ("0.7.4", "0.7.5")
    )

    assert publish.on_crates_io("0.7.5", index)
    assert not publish.on_crates_io("0.7.6", index)
    assert not publish.on_crates_io("0.7", index)


def test_the_index_is_where_crates_io_keeps_xnvme_sys():
    # The sparse index keeps a crate of four or more letters under the first
    # two, then the next two
    assert publish.INDEX == "https://index.crates.io/xn/vm/xnvme-sys"


def test_pypi_uploads_the_sdist_and_skips_a_version_it_has():
    assert publish.upload("pypi", "1.2.3", Path("s.tar.gz"), Path("C.toml")) == [
        "twine",
        "upload",
        "--skip-existing",
        "s.tar.gz",
    ]


def test_crate_uploads_a_version_crates_io_does_not_have():
    index = json.dumps({"name": "xnvme-sys", "vers": "1.2.2"})

    assert publish.upload("crate", "1.2.3", Path("s"), Path("C.toml"), index) == [
        "cargo",
        "publish",
        "--no-verify",
        "--manifest-path",
        "C.toml",
    ]
    assert publish.upload("crate", "1.2.2", Path("s"), Path("C.toml"), index) is None


def test_fetch_uploads_nothing():
    assert publish.upload("fetch", "1.2.3", Path("s"), Path("C.toml"), "") is None


def test_a_version_not_x_y_z_is_an_error():
    run = subprocess.run(
        [sys.executable, str(SCRIPT), "fetch", "v1.2.3"], capture_output=True, text=True
    )

    assert run.returncode == 2
    assert "not a version: 'v1.2.3'" in run.stderr


# A gh that writes the assets of a release, with their .sha256sum, for 'release
# download', and fails with STUB_GH_RC; twine and cargo exit with STUB_RC. Each
# logs its command line to STUB_LOG.
STUBS = {
    "gh": f"""#!{sys.executable}
import hashlib, io, os, sys, tarfile
from pathlib import Path

with open(os.environ["STUB_LOG"], "a") as fd:
    fd.write("gh " + " ".join(sys.argv[1:]) + "\\n")
if os.environ.get("STUB_GH_RC", "0") != "0":
    sys.exit(int(os.environ["STUB_GH_RC"]))
args = sys.argv[1:]
version = args[2][1:]
directory = Path(args[args.index("--dir") + 1])
for name in args[args.index("--pattern") + 1 :: 2]:
    if name.endswith(".sha256sum"):
        continue
    member = (
        "PKG-INFO"
        if name.startswith("xnvme-py-sdist")
        else f"xnvme-{{version}}/rust/xnvme-sys/Cargo.toml"
    )
    with tarfile.open(directory / name, "w:gz") as tar:
        info = tarfile.TarInfo(member)
        tar.addfile(info, io.BytesIO(b""))
    digest = hashlib.sha256((directory / name).read_bytes()).hexdigest()
    (directory / f"{{name}}.sha256sum").write_text(f"{{digest}}  {{name}}\\n")
""",
    "upload": f"""#!{sys.executable}
import os, sys
from pathlib import Path

with open(os.environ["STUB_LOG"], "a") as fd:
    fd.write(Path(sys.argv[0]).name + " " + " ".join(sys.argv[1:]) + "\\n")
sys.exit(int(os.environ.get("STUB_RC", "0")))
""",
}


def stubs_in(tmp_path):
    """The directory of the stubs gh, twine and cargo, and the log they write"""

    stubs = tmp_path / "bin"
    stubs.mkdir(exist_ok=True)
    for name in ("gh", "twine", "cargo"):
        (stubs / name).write_text(STUBS.get(name, STUBS["upload"]))
        (stubs / name).chmod(0o755)
    log = tmp_path / "log"
    log.write_text("")

    return stubs, log


def publish_run(tmp_path, command, **env):
    """Run xnvme_publish.py 'command' 1.2.3 with the stubs first on PATH, and
    the command lines they logged"""

    stubs, log = stubs_in(tmp_path)
    run = subprocess.run(
        [sys.executable, str(SCRIPT), command, "1.2.3", "--dir", str(tmp_path / "out")],
        env=dict(
            os.environ, PATH=f"{stubs}:{os.environ['PATH']}", STUB_LOG=str(log), **env
        ),
        capture_output=True,
        text=True,
    )

    return run, log.read_text().splitlines()


def test_the_assets_are_named_as_the_release_attaches_them():
    assert publish.assets("1.2.3") == (
        "xnvme-py-sdist-v1.2.3.tar.gz",
        "xnvme-1.2.3.tar.gz",
    )


def test_pypi_fetches_the_release_and_uploads_its_sdist(tmp_path):
    run, logged = publish_run(tmp_path, "pypi")
    sdist = tmp_path / "out" / "pypi" / "xnvme-1.2.3.tar.gz"

    assert run.returncode == 0, run.stderr
    assert logged == [
        "gh release download v1.2.3 --repo xnvme/xnvme"
        f" --dir {tmp_path / 'out' / 'assets'}"
        " --pattern xnvme-py-sdist-v1.2.3.tar.gz"
        " --pattern xnvme-py-sdist-v1.2.3.tar.gz.sha256sum"
        " --pattern xnvme-1.2.3.tar.gz --pattern xnvme-1.2.3.tar.gz.sha256sum",
        f"twine upload --skip-existing {sdist}",
    ]


def test_a_failed_upload_fails_the_command(tmp_path):
    run, _ = publish_run(tmp_path, "pypi", STUB_RC="3")

    assert run.returncode == 3


def test_a_failed_download_uploads_nothing(tmp_path):
    run, logged = publish_run(tmp_path, "pypi", STUB_GH_RC="1")

    assert run.returncode == 1
    assert run.stderr.startswith("publish: ")
    assert [line.split()[0] for line in logged] == ["gh"]


def crate_main(tmp_path, monkeypatch, listed, **env):
    """The return code of main() for 'crate 1.2.3', with the stubs first on
    PATH and an index of crates.io that lists the versions 'listed', and the
    command lines the stubs logged"""

    stubs, log = stubs_in(tmp_path)
    index = tmp_path / "index"
    index.write_text(
        "".join(json.dumps({"name": "xnvme-sys", "vers": v}) + "\n" for v in listed)
    )
    monkeypatch.setattr(publish, "INDEX", index.as_uri())
    monkeypatch.setenv("PATH", f"{stubs}:{os.environ['PATH']}")
    monkeypatch.setenv("STUB_LOG", str(log))
    for key, value in env.items():
        monkeypatch.setenv(key, value)
    argv = ["xnvme_publish.py", "crate", "1.2.3", "--dir", str(tmp_path / "out")]
    monkeypatch.setattr(sys, "argv", argv)

    return publish.main(), log.read_text().splitlines()


def test_crate_uploads_xnvme_sys_from_the_source_archive(tmp_path, monkeypatch):
    returncode, logged = crate_main(tmp_path, monkeypatch, ["1.2.2"], STUB_RC="4")
    manifest = tmp_path / "out" / "src" / "xnvme-1.2.3" / "rust/xnvme-sys/Cargo.toml"

    assert returncode == 4
    assert logged[1:] == [f"cargo publish --no-verify --manifest-path {manifest}"]


def test_crate_passes_a_version_crates_io_has(tmp_path, monkeypatch):
    returncode, logged = crate_main(tmp_path, monkeypatch, ["1.2.2", "1.2.3"])

    assert returncode == 0
    assert [line.split()[0] for line in logged] == ["gh"]
