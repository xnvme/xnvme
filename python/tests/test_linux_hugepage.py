"""
The Linux NVMe driver and its ioctl calls to read and write has a somewhat
undocumented limitation of 127 "segments". These segments are determined as
the number of non-contiguous pieces of *physical* memory in a buffer.

If you allocate 1GB with malloc, you'll get contiguous *virtual* memory, but
the physical placement will be determined upon the first write to the allocated
memory.

The kernel will reject an ioctl with more than 127 of these segments. This means
you cannot safely assume to transfer more than 127 * page_size of data.
"""
import ctypes
import io
import mmap
import os
import re
import struct
from ctypes import byref, pointer
from itertools import tee

import conftest
import numpy as np
import pytest
import xnvme.ctypes_bindings.api as xnvme
from conftest import xnvme_parametrize
from utils import buf_and_view, dev_from_params, xnvme_cmd_ctx_cpl_status
from xnvme.ctypes_bindings.api import char_pointer_cast

HUGEPAGE_SIZE = 0
BLK_MAX_SEGMENTS = 128
PAGE_SIZE = 1 << 12  # 4KB
NULL = None

pytest.skip("Needs to be ported from Cython to Ctypes", allow_module_level=True)


def reduce_to_segments(page_frame_number_list, length):
    def pairwise(iterable):
        """s -> (s0,s1), (s1,s2), (s2, s3), ..."""

        a, b = tee(iterable)
        next(b, None)
        return zip(a, b)

    for p1, p2 in pairwise(
        map(lambda x: x[0] - x[1], zip(page_frame_number_list, range(length)))
    ):
        if p1 == p2:
            continue
        yield p1


def get_hugepagesize():
    """Retrieves the hugepagesize from /proc"""

    meminfo_path = "/proc/meminfo"

    if os.path.exists(meminfo_path):
        with open(meminfo_path, "r") as f:
            match = re.search(r"Hugepagesize:\s*(\d+) kB", f.read())
            if match:
                return int(match.groups()[0]) * 1024

    return 0


def get_page_frame_numbers(buf, length):
    # https://www.kernel.org/doc/Documentation/vm/pagemap.txt
    with open(f"/proc/{os.getpid()}/pagemap", "rb") as f:
        for n in range(length // PAGE_SIZE):
            f.seek((buf // PAGE_SIZE + n) * 8)
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


@pytest.fixture(autouse=True)
def prerequisites():
    """
    Ensure that:

    * transparent huge-pages are **not** enabled
    * HUGEPAGE_SIZE is known and non-zero

    In case transparent hugepages are enabled, then disable them by running::

        echo never > /sys/kernel/mm/transparent_hugepage/enabled

    .. todo::
        A script disabling transparent hugepages should probably run before
        running these tests
    """

    with open("/sys/kernel/mm/transparent_hugepage/enabled", "r") as f:
        assert (
            f.read() == "always madvise [never]\n"
        ), "Unexpected state of transparent hugepages"

    HUGEPAGE_SIZE = get_hugepagesize()
    assert HUGEPAGE_SIZE, "Failed determining HUGEPAGE_SIZE"


class TestHugepage:
    @pytest.mark.xnvme_sync(b"nvme")
    @pytest.mark.xnvme_mem(b"posix")
    def test_max_segments_ioctl(self, dev):
        """
        The Linux NVMe driver and its ioctl calls to read and write has a somewhat
        undocumented limitation of 127 "segments". These segments are determined as
        the number of non-contiguous pieces of *physical* memory in a buffer.

        If you allocate 1GB with malloc, you'll get contiguous *virtual* memory, but
        the physical placement will be determined upon the first write to the allocated
        memory.

        The kernel will reject an ioctl with more than 127 of these segments. This means
        you cannot safely assume to transfer more than 127*page_size of data.

        This test purposefully allocates a buffer with more segments to see it break as
        expected, and afterwards an allocation that is not segmented.
        """

        # At least BLK_MAX_SEGMENTS*PAGE_SIZE, but more improves probability to find fragmented memory
        buffer_size = (BLK_MAX_SEGMENTS * 2) * PAGE_SIZE

        # Allocate buffer with maximum segments
        # Strategy:
        # 1. Allocate two buffers of same size -- physical pages aren't assigned yet.
        # 2. Write a zero to every 4k segment of the two allocations.
        # 3. By writing first to the one and then the other, we force the kernel into assigning interleaved pages.
        # 4. We just need one buffer, so we free the second buffer.
        # The resulting buffer should contain roughly as many segments as pages
        for _ in range(256):
            max_segments = xnvme.xnvme_buf_alloc(dev, buffer_size)
            max_segments_memview = np.ctypeslib.as_array(
                ctypes.cast(max_segments.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
                shape=(buffer_size,),
            )
            buf2 = xnvme.xnvme_buf_alloc(dev, buffer_size)
            buf2_memview = np.ctypeslib.as_array(
                ctypes.cast(buf2.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
                shape=(buffer_size,),
            )
            # Obtain maximally entangled qubit pair
            for n in range(buffer_size // PAGE_SIZE):
                # Zero memory and force page allocation
                max_segments_memview[PAGE_SIZE * n] = 0
                buf2_memview[PAGE_SIZE * n] = 0

            xnvme.xnvme_buf_free(dev, buf2)
            segment_count = len(
                list(
                    reduce_to_segments(
                        get_page_frame_numbers(max_segments, buffer_size),
                        buffer_size // PAGE_SIZE,
                    )
                )
            )
            if segment_count >= BLK_MAX_SEGMENTS - 1:
                break
            xnvme.xnvme_buf_free(dev, max_segments)
        else:
            pytest.skip(
                f"The buffer doesn't have more than BLK_MAX_SEGMENTS segments, as it should. The last buffer had {segment_count} segments."
            )

        # Allocate buffer with a 'minimum' of segments -- anything less than BLK_MAX_SEGMENTS will do.
        # Strategy:
        # 1. Allocate a buffer
        # 2. If number of segments is acceptable, then break
        # 3. Otherwise, keep buffer and only deallocate later -- otherwise we'll just reallocate the same pages again
        # 4. Goto 1.
        garbage_list = []
        try:
            for _ in range(256):
                min_segments = xnvme.xnvme_buf_alloc(dev, buffer_size)
                min_segments_memview = np.ctypeslib.as_array(
                    ctypes.cast(
                        min_segments.void_pointer, ctypes.POINTER(ctypes.c_uint8)
                    ),
                    shape=(buffer_size,),
                )
                # Zero memory and force page allocation
                min_segments_memview[:] = 0

                segment_count = len(
                    list(
                        reduce_to_segments(
                            get_page_frame_numbers(min_segments, buffer_size),
                            buffer_size // PAGE_SIZE,
                        )
                    )
                )
                if segment_count < BLK_MAX_SEGMENTS - 1:
                    break
                garbage_list.append(min_segments)
            else:
                assert None, "Failed to allocate buffer"
        finally:
            for b in garbage_list:
                xnvme.xnvme_buf_free(dev, b)

        # Actual transfers
        try:
            # Has to fail because of too many segments
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            lba_nbytes = xnvme.xnvme_dev_get_geo(dev).lba_nbytes
            err = xnvme.xnvme_nvm_write(
                ctx,
                conftest.DEVICE_NSID,
                0,
                buffer_size // lba_nbytes - 1,
                max_segments,
                NULL,
            )
            cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            assert err or cpl, f"err: 0x{err:x}, cpl: 0x{cpl:x}"

            # Has to succeed because of fewer than max segments
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            lba_nbytes = xnvme.xnvme_dev_get_geo(dev).lba_nbytes
            err = xnvme.xnvme_nvm_write(
                ctx,
                conftest.DEVICE_NSID,
                0,
                buffer_size // lba_nbytes - 1,
                min_segments,
                NULL,
            )
            cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"
        finally:
            xnvme.xnvme_buf_free(dev, min_segments)
            xnvme.xnvme_buf_free(dev, max_segments)

    @pytest.mark.parametrize(
        "sync_backend",
        [
            pytest.param("nvme", marks=pytest.mark.xnvme_sync(b"nvme")),
            pytest.param("psync", marks=pytest.mark.xnvme_sync(b"psync")),
        ],
    )
    @pytest.mark.xnvme_mem(b"hugepage")
    @pytest.mark.parametrize(
        "buffer_size",
        [
            (BLK_MAX_SEGMENTS * 2) * PAGE_SIZE,
            HUGEPAGE_SIZE,
            2 * HUGEPAGE_SIZE,
        ],  # Limited by device MDTS!
    )
    def test_max_segments_ioctl_hugepage(
        self, dev, autofreed_buffer, buffer_size, sync_backend
    ):
        # Based on the hugepage implementation, we cannot force the segmentation
        # like on regular pages. We also cannot make transfers large enough on our
        # test systems to have BLK_MAX_SEGMENTS * HUGEPAGE_SIZE buffers.
        # But we still proof the presence of hugepages, and allocating buffers
        # multiple times larger than our 4K page test works.
        buf, buf_memview = autofreed_buffer(dev, buffer_size)

        page_frame_number = next(get_page_frame_numbers(buf, buffer_size))
        assert check_frame_is_hugepage(page_frame_number)

        # Has to succeed because of fewer than max segments
        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        lba_nbytes = xnvme.xnvme_dev_get_geo(dev).lba_nbytes
        err = xnvme.xnvme_nvm_write(
            ctx, conftest.DEVICE_NSID, 0, buffer_size // lba_nbytes - 1, buf, NULL
        )
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"

        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        err = xnvme.xnvme_file_pwrite(ctx, buf, buffer_size, 0)
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"


@pytest.mark.parametrize(
    "sync_backend",
    [
        pytest.param("nvme", marks=pytest.mark.xnvme_sync(b"nvme")),
        pytest.param("psync", marks=pytest.mark.xnvme_sync(b"psync")),
    ],
)
@pytest.mark.parametrize(
    "mem_backend",
    [
        pytest.param("hugepage", marks=pytest.mark.xnvme_mem(b"hugepage")),
        pytest.param("posix", marks=pytest.mark.xnvme_mem(b"posix")),
    ],
)
@pytest.mark.parametrize("pattern", [0b01, 0b11])
@pytest.mark.parametrize("buffer_size", [PAGE_SIZE, 2 * PAGE_SIZE, HUGEPAGE_SIZE])
def test_pwrite_pread(
    dev, autofreed_buffer, buffer_size, sync_backend, mem_backend, pattern
):
    if buffer_size == 0:
        # Hugepage size becomes 0 on unsupported platforms
        pytest.skip("Unsupported buffer size")
    if (
        buffer_size == HUGEPAGE_SIZE
        and mem_backend == "posix"
        and sync_backend == "nvme"
    ):
        pytest.skip("Unsupported buffer size for backend")

    buf, buf_memview = autofreed_buffer(dev, buffer_size)

    page_frame_number = next(get_page_frame_numbers(buf, buffer_size))
    if xnvme.xnvme_dev_get_opts(dev).mem == b"hugepage":
        assert check_frame_is_hugepage(page_frame_number)
    else:
        assert not check_frame_is_hugepage(page_frame_number)

    # Write to device with changing bit-pattern
    for n in range(4):
        buf_memview[:] = pattern << (2 * n)  # Just some changing bit-pattern

        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        err = xnvme.xnvme_file_pwrite(ctx, buf, buffer_size, buffer_size * n)
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"

    # Read out with regular Python file-read to verify
    buf_verify = mmap.mmap(-1, buffer_size)
    # os.O_DIRECT is critical, otherwise, we read invalid page cache data!
    # We are opening first with os.open to get O_DIRECT, and then converting it
    # to a regular Python file-object to get context-manager and .readinto()
    with io.open(
        os.open(conftest.DEVICE_PATH, os.O_DIRECT | os.O_RDONLY), "rb", buffering=0
    ) as f:
        for n in range(4):
            bit_pattern = pattern << (2 * n)  # Just some changing bit-pattern

            f.seek(buffer_size * n)
            assert f.readinto(buf_verify) == buffer_size
            assert np.all(np.frombuffer(buf_verify, dtype=np.uint8) == bit_pattern)
    buf_verify.close()

    # Read out with xnvme_file_pread to verify
    for n in range(4):
        buf_memview[:] = 0
        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        err = xnvme.xnvme_file_pread(ctx, buf, buffer_size, buffer_size * n)
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"

        bit_pattern = pattern << (2 * n)  # Same bit-pattern as write
        assert np.all(buf_memview == bit_pattern), np.where(buf_memview != bit_pattern)


@pytest.mark.parametrize(
    "sync_backend",
    [
        pytest.param("nvme", marks=pytest.mark.xnvme_sync(b"nvme")),
        pytest.param("psync", marks=pytest.mark.xnvme_sync(b"psync")),
    ],
)
@pytest.mark.parametrize(
    "mem_backend",
    [
        pytest.param("hugepage", marks=pytest.mark.xnvme_mem(b"hugepage")),
        pytest.param("posix", marks=pytest.mark.xnvme_mem(b"posix")),
    ],
)
@pytest.mark.parametrize("pattern", [0b01, 0b11])
@pytest.mark.parametrize("buffer_size", [PAGE_SIZE, 2 * PAGE_SIZE, HUGEPAGE_SIZE])
def test_nvm_write_read(
    dev, autofreed_buffer, buffer_size, sync_backend, mem_backend, pattern
):
    if buffer_size == 0:
        # Hugepage size becomes 0 on unsupported platforms
        pytest.skip("Unsupported buffer size")
    if (
        buffer_size == HUGEPAGE_SIZE
        and mem_backend == "posix"
        and sync_backend == "nvme"
    ):
        pytest.skip("Unsupported buffer size for backend")

    buf, buf_memview = autofreed_buffer(dev, buffer_size)

    page_frame_number = next(get_page_frame_numbers(buf, buffer_size))
    if xnvme.xnvme_dev_get_opts(dev).mem == b"hugepage":
        assert check_frame_is_hugepage(page_frame_number)
    else:
        assert not check_frame_is_hugepage(page_frame_number)

    lba_nbytes = xnvme.xnvme_dev_get_geo(dev).lba_nbytes

    # Write to device with changing bit-pattern
    for n in range(4):
        buf_memview[:] = pattern << (2 * n)  # Just some changing bit-pattern

        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        slba = (n * buffer_size) // lba_nbytes
        nlb = (buffer_size // lba_nbytes) - 1
        err = xnvme.xnvme_nvm_write(ctx, conftest.DEVICE_NSID, slba, nlb, buf, NULL)
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"

    # Read out with regular Python file-read to verify
    buf_verify = mmap.mmap(-1, buffer_size)
    # os.O_DIRECT is critical, otherwise, we read invalid page cache data!
    # We are opening first with os.open to get O_DIRECT, and then converting it
    # to a regular Python file-object to get context-manager and .readinto()
    with io.open(
        os.open(conftest.DEVICE_PATH, os.O_DIRECT | os.O_RDONLY), "rb", buffering=0
    ) as f:
        for n in range(4):
            bit_pattern = pattern << (2 * n)  # Just some changing bit-pattern

            f.seek(buffer_size * n)
            assert f.readinto(buf_verify) == buffer_size
            assert np.all(np.frombuffer(buf_verify, dtype=np.uint8) == bit_pattern)
    buf_verify.close()

    # Read out with xnvme_file_pread to verify
    for n in range(4):
        buf_memview[:] = 0

        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        slba = (n * buffer_size) // lba_nbytes
        nlb = (buffer_size // lba_nbytes) - 1
        err = xnvme.xnvme_nvm_read(ctx, conftest.DEVICE_NSID, slba, nlb, buf, NULL)
        cpl = xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        assert not (err or cpl), f"err: 0x{err:x}, cpl: 0x{cpl:x}"

        bit_pattern = pattern << (2 * n)  # Same bit-pattern as write
        assert np.all(buf_memview == bit_pattern), np.where(buf_memview != bit_pattern)
