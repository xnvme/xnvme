"""
This tests whether:

* It is possible to load the shared library
* It is possible to link with the shared library
* It is possible to link with the static library

It does so by compiling programs and running them, and additionally, checking
the size of the result binraries.
"""
from pathlib import Path

from cijoe.core.resources import get_resources


def prep_sources(cijoe):
    """
    Transfers sources to target and returns a dict with paths to them
    """

    resources = get_resources()

    tmp = Path("/tmp")
    paths = {
        "loading": tmp / "xnvme_loading.c",
        "linking": tmp / "xnvme_linking.c",
    }

    for ltype, path in paths.items():
        cijoe.put(resources["auxiliary"][f"xnvme_{ltype}"].path, paths[ltype])

    return paths


def list_libraries(cijoe):
    """Returns a list of Paths to xNVMe libraries"""

    suffix = {
        ".so": "shared",
        ".dylib": "shared",
        ".dll": "shared",
        ".a": "static",
    }

    err, state = cijoe.run("pkg-config xnvme --variable=libdir")
    assert not err

    libdir = state.output().strip()

    err, state = cijoe.run(f"ls {libdir}")
    assert not err

    paths = [
        Path(libdir) / line
        for line in state.output().split()
        if "libxnvme" in Path(line.strip()).stem
    ]

    return {suffix[lib.suffix]: lib for lib in paths}


def test_library_consumption(cijoe):
    """
    Verify that xNVMe is consumable via dynamic loading, shared linking, static linking
    """

    compiler = (
        cijoe.config.options.get("build", {}).get("compiler", {}).get("bin", "gcc")
    )

    usages = [
        ("shared", "loading", "pkg-config xnvme --cflags"),
        ("shared", "linking", "pkg-config xnvme --cflags --libs"),
        ("static", "linking", "pkg-config xnvme --cflags --libs --static"),
    ]
    srcs = prep_sources(cijoe)
    libs = list_libraries(cijoe)

    sizes = []

    for ltype, linkorload, pkgconf in usages:
        # Get the compiler-flags from pkg-config
        err, state = cijoe.run(pkgconf)
        assert not err

        # Replace "-lxnvme", in flags, with absolute path to "libxnvme.a"
        flags = state.output().strip()
        if ltype == "static":
            flags = flags.replace("-lxnvme", f"{libs['static']}")

        # Construct a path to the executable
        bin = Path("/tmp") / f"xnvme_{linkorload}_{ltype}"

        # Add binary-path as -o flag
        flags = f"{flags} -o {bin}"

        execbin = f"{bin}"
        if linkorload == "loading":
            flags = f"-ldl {flags}"
            execbin = f"{execbin} {libs['shared']}"

        # Build source
        err, _ = cijoe.run(f"{compiler} {srcs[linkorload]} {flags}")
        assert not err

        # Grab the size in bytes of the built library
        err, state = cijoe.run(f"wc -c {bin}")
        assert not err
        sizes.append(int(state.output().strip().split(" ")[0]))

        # Run it
        err, state = cijoe.run(execbin)
        assert not err

    assert len(list(set(sizes))) == 3, "Binaries have the same size; they should not"
