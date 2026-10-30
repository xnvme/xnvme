from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).parents[2]
VERIFY = yaml.safe_load((ROOT / ".github" / "workflows" / "verify.yml").read_text())
BUILD_LINUX = yaml.safe_load(VERIFY["jobs"]["matrices"]["env"]["BUILD_LINUX"])
DEPS = yaml.safe_load((ROOT / "toolbox" / "pkgs" / "emitter" / "deps.yaml").read_text())


def emitted():
    """The (os, ver) pairs deps.yaml emits a script for"""

    platforms = DEPS["platforms"] if isinstance(DEPS, dict) else DEPS
    return {
        (platform["name"], str(ver))
        for platform in platforms
        if isinstance(platform, dict) and "name" in platform and "ver" in platform
        for ver in platform["ver"]
    }


@pytest.mark.parametrize("entry", BUILD_LINUX, ids=lambda e: f"{e['os']}-{e['ver']}")
def test_every_build_has_its_pkgs_script(entry):
    assert (ROOT / "toolbox" / "pkgs" / f"{entry['os']}-{entry['ver']}.sh").is_file()


def test_every_pkgs_script_is_emitted_from_deps():
    scripts = {
        path.stem
        for path in (ROOT / "toolbox" / "pkgs").glob("*.sh")
        if not path.stem.endswith("-build") and path.stem not in ("default", "docgen")
    }

    names = {f"{os}-{ver}" for os, ver in emitted()}
    assert scripts - names == set()


def test_nixos_builds_its_package_in_its_deps_image():
    build = yaml.safe_load((ROOT / ".github" / "workflows" / "build.yml").read_text())
    run = "\n".join(step.get("run", "") for step in build["jobs"]["nixos"]["steps"])

    assert "ghcr.io/xnvme/xnvme-deps-nixos-latest:main" in run
    assert "toolbox/pkgs/nixos-latest-build.sh" in run
    assert ("nixos", "latest") in emitted()


def test_every_specialised_template_is_for_an_emitted_platform():
    templates = ROOT / "toolbox" / "pkgs" / "emitter" / "templates"
    names = {f"{os}-{ver}" for os, ver in emitted()}

    orphans = []
    for path in templates.glob("*.sh.jinja"):
        stem = path.name.removesuffix(".sh.jinja")
        comp, *tail = stem.split("-")
        specialised = "-".join(tail[1:])
        if specialised and specialised not in names:
            orphans.append(path.name)

    assert orphans == []
