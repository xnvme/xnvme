#!/usr/bin/env python3
from xnvme import CAPI as capi

def main():
    """Example entry point"""

    dev = capi.xnvme_dev_open(b"/dev/nvme0n1")
    capi.xnvme_dev_pr(dev, 0x0)

    geo = capi.xnvme_dev_get_geo(dev)
    if not geo:
        return

    capi.xnvme_geo_pr(geo, 0x0)

if __name__ == "__main__":
    main()
