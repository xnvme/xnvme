"""
    The xnvme.library_loader provides a single method to dynamically load the xNVMe
    shared library. This intent is that the loader is invoked by the
    'xnvme.ctypes_bindings' on package-initialization, such that the user can directly
    get a library-handle like so:

        from xnvme.ctypes_bindings import capi

    This is useful since usage of the library handle can have multiple entry points.
"""
import ctypes
import ctypes.util
import os
import platform
import subprocess

SHARED_EXT = {
    "linux": "so",
    "windows": "dll",
    "darwin": "dylib",
}


def search_paths():
    """Search the system for the shared library emitting paths for ctypes.CDLL()"""

    for search in ["xnvme-shared", "xnvme_shared"]:
        path = ctypes.util.find_library(search)
        if path:
            yield path

    try:
        proc = subprocess.run(
            ["pkg-config", "xnvme", "--variable=libdir"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not proc.returncode:
            ext = SHARED_EXT.get(platform.system().lower(), "so")

            yield os.path.join(
                proc.stdout.decode("utf-8").strip(), f"libxnvme-shared.{ext}"
            )
    except subprocess.CalledProcessError:
        pass


def load():
    """Dynamically load the xNVMe shared library"""

    for spath in search_paths():
        try:
            lib = ctypes.CDLL(spath)
            if lib:
                return lib
        except OSError:
            continue

    return None
