import ctypes

import conftest
import numpy as np
import pytest
import xnvme.cython_bindings as xnvme
from utils import (
    BLK_MAX_SEGMENTS,
    HUGEPAGE_SIZE,
    NULL,
    PAGE_SIZE,
    check_frame_is_hugepage,
    get_page_frame_numbers,
    reduce_to_segments,
)


class MemorySegmentException(Exception):
    pass


def is_memory_segment_exception(err, *args):
    return issubclass(err[0], MemorySegmentException)


@pytest.mark.skipif(
    conftest.BACKEND != b"linux", reason="Hugepages are only supported on Linux"
)
class TestHugepage:
    @pytest.fixture(autouse=True)
    def _prerequisites(self):
        with open("/sys/kernel/mm/transparent_hugepage/enabled", "r") as f:
            assert f.read() == "always madvise [never]\n"

    @pytest.mark.flaky(
        max_runs=3, min_passes=1, rerun_filter=is_memory_segment_exception
    )
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
            raise MemorySegmentException(
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
        buf, buf_memview = autofreed_buffer(buffer_size)

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
