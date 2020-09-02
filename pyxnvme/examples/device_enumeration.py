#!/usr/bin/env python3
"""
    Showing how to enumerated devices via the ctypes-wrapped xNVMe C API
"""
from ctypes import byref, POINTER
import xnvme

def main():
    """Enumerate devices on the system"""

    arg = POINTER(xnvme.Enumeration)()

    xnvme.CAPI.xnvme_enumerate(byref(arg), 0x0)

    xnvme.CAPI.xnvme_enumeration_pr(arg, 0x0)

if __name__ == "__main__":
    main()
