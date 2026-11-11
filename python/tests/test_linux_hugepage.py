"""
I/O from buffers of the memory backends 'hugepage' and 'posix' on Linux

Each test checks, through /proc/self/pagemap and /proc/kpageflags, that the
buffer xnvme_buf_alloc() returns is backed by a hugepage exactly when the
backend is 'hugepage', writes a pattern through xNVMe, reads it back with
O_DIRECT around xNVMe and then through it. Reading page frames needs root.
"""

import ctypes
import io
import mmap
import os
import re
import struct

import numpy
import pytest
import xnvme
from conftest import xnvme_parametrize
from utils import buf_and_view, dev_from_params, xnvme_cmd_ctx_cpl_status

PAGE_SIZE = 1 << 12

# Size of the file the file-tests create when the configured one is missing
FILE_NBYTES = 64 << 20

# Writes per test, each of a buffer of its own pattern, one after the other
NWRITES = 4


def hugepage_nbytes():
    """The size of a hugepage, from /proc/meminfo; 0 when unknown"""

    with open("/proc/meminfo") as meminfo:
        match = re.search(r"Hugepagesize:\s*(\d+) kB", meminfo.read())

    return int(match.group(1)) * 1024 if match else 0


def buffer_nbytes(size):
    """The size in bytes of the buffer named 'size'"""

    return {"page": PAGE_SIZE, "pages": 2 * PAGE_SIZE, "hugepage": hugepage_nbytes()}[
        size
    ]


def is_hugepage(addr):
    """Whether the page at virtual address 'addr' is a hugepage, and not a THP"""

    kpf_huge, kpf_thp = 17, 22  # include/uapi/linux/kernel-page-flags.h

    with open("/proc/self/pagemap", "rb") as pagemap:
        pagemap.seek(addr // PAGE_SIZE * 8)
        (entry,) = struct.unpack("<Q", pagemap.read(8))
    assert entry & (1 << 63), "page not present"
    pfn = entry & ((1 << 55) - 1)
    assert pfn, "no page frame number; reading it needs root"

    with open("/proc/kpageflags", "rb") as kpageflags:
        kpageflags.seek(pfn * 8)
        (flags,) = struct.unpack("<Q", kpageflags.read(8))

    return bool(flags & (1 << kpf_huge)) and not flags & (1 << kpf_thp)


def skip_unless_able(be_opts, nbytes):
    if os.geteuid():
        pytest.skip(reason="reading page frames needs root")
    if not nbytes:
        pytest.skip(reason="the size of a hugepage is unknown")
    if be_opts["mem"] == "hugepage":
        with open("/proc/sys/vm/nr_hugepages") as nr_hugepages:
            if not int(nr_hugepages.read()):
                pytest.skip(reason="no hugepages are reserved")


def check(err, ctx, name):
    status = xnvme_cmd_ctx_cpl_status(ctypes.pointer(ctx))
    assert not (err or status), f"{name}(): err({err}), status(0x{status:x})"


def pattern(n):
    return 0x5A ^ (n * 0x11)


def check_backing(buf, be_opts):
    huge = is_hugepage(buf)
    if be_opts["mem"] == "hugepage":
        assert huge, "a buffer of mem 'hugepage' is not backed by a hugepage"
    else:
        assert not huge, f"a buffer of mem '{be_opts['mem']}' is backed by a hugepage"


def check_with_odirect(path, nbytes):
    """Read the NWRITES buffers back from 'path' with O_DIRECT, around xNVMe"""

    verify = mmap.mmap(-1, nbytes)
    with io.open(os.open(path, os.O_DIRECT | os.O_RDONLY), "rb", buffering=0) as f:
        for n in range(NWRITES):
            f.seek(n * nbytes)
            assert f.readinto(verify) == nbytes
            assert numpy.all(numpy.frombuffer(verify, dtype=numpy.uint8) == pattern(n))
    verify.close()


@pytest.mark.parametrize("size", ["page", "pages", "hugepage"])
@xnvme_parametrize(labels=["file"], opts=["mem"])
def test_pwrite_pread(cijoe, device, be_opts, cli_args, size):
    """xnvme_file_pwrite() and xnvme_file_pread() of a file, from each backend"""

    nbytes = buffer_nbytes(size)
    skip_unless_able(be_opts, nbytes)

    path = device["uri"]
    created = not os.path.exists(path)
    if created:
        with open(path, "wb") as f:
            f.truncate(FILE_NBYTES)

    dev = dev_from_params(device, be_opts)
    assert dev
    try:
        buf, view = buf_and_view(dev, nbytes)
        check_backing(buf, be_opts)

        for n in range(NWRITES):
            view[:] = pattern(n)
            ctx = xnvme.XnvmeCmdCtx()
            assert not xnvme.xnvme_dev_get_cmd_ctx(dev, ctypes.byref(ctx))
            err = xnvme.xnvme_file_pwrite(ctx, buf, nbytes, n * nbytes)
            check(err, ctx, "xnvme_file_pwrite")

        check_with_odirect(path, nbytes)

        for n in range(NWRITES):
            view[:] = 0
            ctx = xnvme.XnvmeCmdCtx()
            assert not xnvme.xnvme_dev_get_cmd_ctx(dev, ctypes.byref(ctx))
            err = xnvme.xnvme_file_pread(ctx, buf, nbytes, n * nbytes)
            check(err, ctx, "xnvme_file_pread")
            assert numpy.all(view == pattern(n)), f"write {n} read back wrong"

        xnvme.xnvme_buf_free(dev, buf)
    finally:
        xnvme.xnvme_dev_close(dev)
        if created:
            os.unlink(path)


@pytest.mark.parametrize("size", ["page", "pages", "hugepage"])
@xnvme_parametrize(labels=["bdev"], opts=["mem", "sync"])
def test_nvm_write_read(cijoe, device, be_opts, cli_args, size):
    """xnvme_nvm_write() and xnvme_nvm_read() of a block device, from each backend"""

    if be_opts["label"] != ["bdev"] or be_opts["sync"] not in ["nvme", "block"]:
        pytest.skip(reason="the test reads a kernel block device back itself")

    nbytes = buffer_nbytes(size)
    skip_unless_able(be_opts, nbytes)

    dev = dev_from_params(device, be_opts)
    assert dev
    try:
        geo = xnvme.xnvme_dev_get_geo(dev).contents
        if be_opts["sync"] == "nvme" and nbytes > geo.mdts_nbytes:
            pytest.skip(reason=f"{nbytes} bytes exceed MDTS, {geo.mdts_nbytes} bytes")
        nsid = xnvme.xnvme_dev_get_nsid(dev)
        nlb = nbytes // geo.lba_nbytes - 1

        buf, view = buf_and_view(dev, nbytes)
        check_backing(buf, be_opts)

        for n in range(NWRITES):
            view[:] = pattern(n)
            ctx = xnvme.XnvmeCmdCtx()
            assert not xnvme.xnvme_dev_get_cmd_ctx(dev, ctypes.byref(ctx))
            slba = n * nbytes // geo.lba_nbytes
            err = xnvme.xnvme_nvm_write(ctx, nsid, slba, nlb, buf, None)
            check(err, ctx, "xnvme_nvm_write")

        check_with_odirect(device["uri"], nbytes)

        for n in range(NWRITES):
            view[:] = 0
            ctx = xnvme.XnvmeCmdCtx()
            assert not xnvme.xnvme_dev_get_cmd_ctx(dev, ctypes.byref(ctx))
            slba = n * nbytes // geo.lba_nbytes
            err = xnvme.xnvme_nvm_read(ctx, nsid, slba, nlb, buf, None)
            check(err, ctx, "xnvme_nvm_read")
            assert numpy.all(view == pattern(n)), f"write {n} read back wrong"

        xnvme.xnvme_buf_free(dev, buf)
    finally:
        xnvme.xnvme_dev_close(dev)
