#!/usr/bin/env python3
from ctypes import byref, POINTER
from xnvme import CAPI as capi
import xnvme

def main():
    """Enumerate devices on the system"""

    arg = POINTER(xnvme.Enumeration)()

    capi.xnvme_enumerate(byref(arg), 0x0)

    capi.xnvme_enumeration_pr(arg, 0x0)

if __name__ == "__main__":
    main()
