import ctypes
import ctypes.util

xnvme_shared_lib = ctypes.util.find_library("xnvme-shared")
capi = ctypes.CDLL(xnvme_shared_lib)


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
