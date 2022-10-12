#!/usr/bin/env python3
import argparse
import os
from io import StringIO

import autopxd

regex = [
    r"s/SLIST_ENTRY\(xnvme_cmd_ctx\)/struct{struct xnvme_cmd_ctx *sle_next;}/g",
    r"s/SLIST_HEAD\(, xnvme_cmd_ctx\)/struct{struct xnvme_cmd_ctx *slh_first;}/g",
    r"s/struct iovec\s?\*/void */g",
    # NOTE: Cython doesn't support arrays without length specified
    r"s/xnvme_be_attr item\[\]/xnvme_be_attr item[1]/g",
    r"s/xnvme_ident entries\[\]/xnvme_ident entries[1]/g",
    r"s/uint8_t storage\[\]/uint8_t storage[1]/g",
    r"s/uint16_t sid\[\]/uint16_t sid[1]/g",
]


def generate_pxd(workspace_root):
    """Returns the content of a Cython pyx-file mapping the xNVMe API"""

    c_include_path = os.path.join(workspace_root, "include")

    extra_cpp_args = [
        f"-I{c_include_path}",
        f'-I{os.path.join(workspace_root, "third-party/windows/")}',
    ]

    pxd_contents = StringIO()
    pxd_contents.write(
        """
from libc.stdio cimport FILE

ctypedef int off_t

ctypedef bint bool

"""
    )

    # TODO: Order is somewhat significant. libxnvme_spec.h has to be on top because of forward declarations
    xnvme_header_files = [
        "libxnvme_spec.h",
        "libxnvme_nvm.h",
        "libxnvme_libconf.h",
        "libxnvme_lba.h",
        "libxnvme_ident.h",
        "libxnvme_geo.h",
        "libxnvme_file.h",
        "libxnvme_spec_fs.h",
        "libxnvme_spec_pp.h",
        "libxnvme_znd.h",
        "libxnvme_pp.h",
        "libxnvme_be.h",
        "libxnvme_adm.h",
        "libxnvme_buf.h",
        "libxnvme_util.h",
        "libxnvmec.h",
        "libxnvme.h",
        "libxnvme_dev.h",
        "libxnvme_ver.h",
    ]
    for h_file in xnvme_header_files:
        h_path = os.path.join(c_include_path, h_file)
        with open(h_path, "r") as f_in:
            whitelist = {
                os.path.join(autopxd.BUILTIN_HEADERS_DIR, d)
                for d in os.listdir(autopxd.BUILTIN_HEADERS_DIR)
            }
            if os.path.isdir(autopxd.DARWIN_HEADERS_DIR):
                whitelist |= {
                    os.path.join(autopxd.DARWIN_HEADERS_DIR, d)
                    for d in os.listdir(autopxd.DARWIN_HEADERS_DIR)
                }
            whitelist |= {
                "<stdin>",  # Allow declarations from the file given by h_path
            }

            pxd_contents.write("\n")
            pxd_contents.write(
                autopxd.translate(
                    f_in.read(),
                    h_file,
                    extra_cpp_args,
                    debug=False,
                    regex=regex,
                    whitelist=whitelist,
                )
            )

    pxd_contents.seek(0)
    return pxd_contents


def parse_args():
    """Parse command-line arguments"""

    parser = argparse.ArgumentParser(
        description="Generate Cython .pxd mapping to xNVMe C API",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--src-root",
        help="Path to source root; xNVMe git-repository or extracted source archive",
        required=True,
        type=str,
    )

    return parser.parse_args()


def main(args):
    """Generate Cython .pxd for the given xNVMe git-repository or extracted source archive"""

    pxd_path = os.path.join(
        args.src_root,
        "python",
        "xnvme-cy-header",
        "xnvme",
        "cython_header",
        "libxnvme.pxd",
    )
    with open(pxd_path, "w") as f_out:
        f_out.write(generate_pxd(args.src_root).read())


if __name__ == "__main__":
    main(parse_args())
