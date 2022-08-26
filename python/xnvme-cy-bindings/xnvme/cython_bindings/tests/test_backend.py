import io
import mmap
import os

import conftest
import numpy as np
import pytest
import xnvme.cython_bindings as xnvme
from utils import (
    HUGEPAGE_SIZE,
    NULL,
    PAGE_SIZE,
    check_frame_is_hugepage,
    get_page_frame_numbers,
)

enumerated_devices = 0


@pytest.mark.xfail(
    conftest.BACKEND == b"spdk",
    reason="SPDK cannot handle a dev open/close followed by enumerate",
)
@pytest.mark.skipif(
    conftest.BACKEND == b"ramdisk",
    reason="ramdisk doesn't support enumeration",
)
def test_enum_spdk_fail(opts):
    def callback_func(dev, cb_args):
        global enumerated_devices
        print(xnvme.xnvme_dev_get_ident(dev).to_dict(), cb_args)
        enumerated_devices += 1
        return xnvme.XNVME_ENUMERATE_DEV_CLOSE

    # Open/close device. This will make it disappear from the enumeration on SPDK
    device_path = conftest.DEVICE_PATH
    device = xnvme.xnvme_dev_open(device_path, opts)
    device = xnvme.xnvme_dev_close(device)

    blank_opts = xnvme.xnvme_opts()
    xnvme.xnvme_enumerate(None, blank_opts, callback_func, "Awesome context!")
    assert enumerated_devices > 0


# @pytest.mark.xnvme_admin(b"block")
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

    buf, buf_memview = autofreed_buffer(buffer_size)

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

    buf, buf_memview = autofreed_buffer(buffer_size)

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
