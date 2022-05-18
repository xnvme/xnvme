import ctypes
import ctypes.util
import os
import subprocess


def load_capi():
    """Attempt to load the library via ctypes"""

    def search_paths():
        """Generate paths to try and load"""

        for search in ["xnvme-shared", "xnvme_shared"]:
            path = ctypes.util.find_library(search)
            if path:
                yield path

        try:
            proc = subprocess.run(
                ["pkg-config", "xnvme", "--variable=libdir"],
                check=True,
                capture_output=True,
            )
            if not proc.returncode:
                yield os.path.join(
                    proc.stdout.decode("utf-8").strip(), "libxnvme-shared.so"
                )
        except subprocess.CalledProcessError:
            pass

    for spath in search_paths():
        try:
            lib = ctypes.CDLL(spath)
            if lib:
                return lib
        except OSError:
            continue

    return None


capi = load_capi()


class Ident(ctypes.Structure):
    """Struct containing device identifiers"""

    _fields_ = [
        ("uri", ctypes.c_char * 384),
        ("dtype", ctypes.c_uint32),
        ("nsid", ctypes.c_uint32),
        ("csi", ctypes.c_uint8),
        ("rsvd", ctypes.c_uint8 * 3),
    ]


class Dev(ctypes.Structure):
    """Struct representing the device"""

    _fields_: list[tuple] = []


def enum_cb(dev):
    """Callback function invoked for each device"""

    get_ident = capi.xnvme_dev_get_ident
    get_ident.restype = ctypes.POINTER(Ident)
    capi.xnvme_ident_pr(get_ident(dev), 0x0)

    return 1


def main():
    """Enumerate devices on the system"""

    enum_cb_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(Dev))
    capi.xnvme_enumerate(None, None, enum_cb_type(enum_cb), None)


if __name__ == "__main__":
    main()
