#!/usr/bin/env python3
import argparse
import os


def parse_args():
    """Parse command-line arguments"""

    parser = argparse.ArgumentParser(
        description="Patch the given xnvme.ctypes_mapping",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--path",
        help="Path to ctypes_mapping.py",
        required=True,
        type=str,
    )

    return parser.parse_args()


SNIPPETS = {
    "import": ("import ctypes\n" "import sys\n" "\n" "from . import library_loader"),
    "loader": (
        "_libraries['xnvme'] = library_loader.load()\n"
        "is_loaded = _libraries['xnvme'] is not None\n"
        "_libraries['xnvme'] = _libraries['xnvme' ]"
        " if _libraries['xnvme'] else FunctionFactoryStub()"
    ),
    "guard": (
        "def guard_unloadable():\n"
        '   """Print error and do sys.exit(1) when library is not loadable"""\n'
        "\n"
        "   if is_loaded:\n"
        "       return\n"
        "\n"
        '   print("FAILED: library is not loadable; perhaps install xNVMe?")\n'
        "   sys.exit(1)\n"
    ),
}


def main(args):
    """Patch the given auto-generated ctypes-mapping"""

    path = os.path.abspath(os.path.expandvars(os.path.expanduser(args.path)))
    with open(path) as ctfile:
        code = ctfile.read()

    # Vanity, change the "FIXNVME_STUB" to "xnvme", with clang2py, the basename of the
    # shared library is indented to be used as index since it can map multple library
    # into the same file of auto-generated mappings. However, since xNVMe provides
    # everything in a single library then just change this to 'xnvme'
    code = code.replace("FIXME_STUB", "xnvme")

    # Replace the FactoryStub/direct ctypes.CDLL load with
    # 'xnvme.ctypes_mapping.library_loader' and provide a variable to probe for state
    code = code.replace("import ctypes", SNIPPETS["import"])
    code = code.replace(
        "_libraries['xnvme'] = FunctionFactoryStub()", SNIPPETS["loader"]
    )

    # Add a function to invoke in order to exit and provide the samme error-message
    # where it is attempted
    code += SNIPPETS["guard"]

    # The following two mapping-changes are a bit controversial, in a general
    # code-generator case removing the 'struct' and 'union' prefix is likely to cause
    # namespace collision.  For xNVMe it does not, which allows for a less verbose API
    # mapping.
    code = code.replace("struct_xnvme", "xnvme")
    code = code.replace("union_xnvme", "xnvme")

    # The entities in the 'skiplist' are names of 'static inline' function and thus
    # unavailable via the shared library, when writing the modified ctypes_mapping to
    # file we skip writing them.
    skiplist = ["xnvme_cmd_ctx_set_cb", "xnvme_cmd_ctx_cpl_status"]

    with open(path, "w") as pfile:
        for line in code.splitlines():
            if [x for x in skiplist if x in line]:
                continue

            pfile.write(line)
            pfile.write("\n")


if __name__ == "__main__":
    main(parse_args())
