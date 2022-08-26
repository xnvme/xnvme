import os
import re
import struct
from itertools import tee

import xnvme.cython_bindings as xnvme

NULL = xnvme.xnvme_void_p(0)
UINT16_MAX = 0xFFFF

PAGE_SIZE = 1 << 12  # 4KB

BLK_MAX_SEGMENTS = 128

HUGEPAGE_SIZE = 0
meminfo_path = "/proc/meminfo"
if os.path.exists(meminfo_path):
    with open(meminfo_path, "r") as f:
        match = re.search(r"Hugepagesize:\s*(\d+) kB", f.read())
        if match:
            HUGEPAGE_SIZE = int(match.groups()[0]) * 1024


def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)


def reduce_to_segments(page_frame_number_list, length):
    for p1, p2 in pairwise(
        map(lambda x: x[0] - x[1], zip(page_frame_number_list, range(length)))
    ):
        if p1 == p2:
            continue
        yield p1


def get_page_frame_numbers(buf, length):
    # https://www.kernel.org/doc/Documentation/vm/pagemap.txt
    with open(f"/proc/{os.getpid()}/pagemap", "rb") as f:
        for n in range(length // PAGE_SIZE):
            f.seek((buf.void_pointer // PAGE_SIZE + n) * 8)
            (page_entry,) = struct.unpack("<Q", f.read(8))
            assert page_entry & (1 << 63), "Page not allocated?"  # PAGEMAP_PRESENT
            assert not page_entry & (1 << 62), "Page swapped"
            yield page_entry & 0x007F_FFFF_FFFF_FFFF  # PAGEMAP_PFN


def check_frame_is_hugepage(page_frame_number):
    with open("/proc/kpageflags", "rb") as f:
        dtype_size = 8  # uint64
        f.seek(page_frame_number * dtype_size)
        (kernel_page_flags,) = struct.unpack("<Q", f.read(dtype_size))

        # Bitmasks from: include/uapi/linux/kernel-page-flags.h
        KPF_HUGE = 17  # Huge Page
        KPF_THP = 22  # Transparent Huge Page

        if not (kernel_page_flags & (1 << KPF_HUGE)):
            # The allocation isn't a hugepage!
            return False

        if kernel_page_flags & (1 << KPF_THP):
            # Don't use transparent hugepage for this test!
            return False

    return True
