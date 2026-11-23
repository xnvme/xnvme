import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).parents[2]
sys.path.insert(0, str(ROOT / "toolbox"))
spec = importlib.util.spec_from_file_location(
    "xnvmec_generator", ROOT / "toolbox" / "xnvmec_generator.py"
)
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)
release = importlib.util.spec_from_file_location(
    "xnvme_release", ROOT / "toolbox" / "xnvme_release.py"
)
xnvme_release = importlib.util.module_from_spec(release)
release.loader.exec_module(xnvme_release)


def test_the_man_pages_name_the_version_of_the_tree():
    assert generator.release() == f"xNVMe v{xnvme_release.version_of(ROOT)}"


def test_a_word_with_a_space_is_one_argument():
    """run() takes a list of words, as txt2man takes '-r', 'xNVMe vX.Y.Z'"""
    out, _, rcode = generator.run(["printf", "%s|", "xNVMe v1.2.3"])

    assert rcode == 0
    assert out == "xNVMe v1.2.3|"
