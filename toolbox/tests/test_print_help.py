import importlib.util
from pathlib import Path
from types import SimpleNamespace

SCRIPT = Path(__file__).parents[2] / "toolbox" / "print_help.py"
spec = importlib.util.spec_from_file_location("print_help", SCRIPT)
print_help = importlib.util.module_from_spec(spec)
spec.loader.exec_module(print_help)

MAKEFILE = """define build-help
# Build it
endef

define SOME_SCRIPT
import sys
endef

define clean-help
# Clean it
endef
"""


def test_only_help_defines_are_targets(tmp_path):
    (tmp_path / "Makefile").write_text(MAKEFILE)
    args = print_help.gen_help(SimpleNamespace(repos=str(tmp_path)))
    assert args.help == {"build": [" Build it"], "clean": [" Clean it"]}


def test_the_makefile_prints(capsys):
    args = print_help.gen_help(SimpleNamespace(repos=str(SCRIPT.parents[1])))
    args.no_colorize = True
    args.verbose = False
    print_help.print_help(args)
    assert "rpm-dist" in capsys.readouterr().out
