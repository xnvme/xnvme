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
