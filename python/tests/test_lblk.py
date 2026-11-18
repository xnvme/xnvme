"""
Logical-block commands through the bindings: writes scattered over a range and
read back, Simple Copy of an array of source ranges, and Write Zeroes
"""

import ctypes

import numpy
import pytest
import xnvme
from conftest import xnvme_parametrize
from utils import buf_and_view, dev_from_params

# Blocks written and read back one by one, and copied
NBLOCKS = 8

# The distance, in LBAs, between the blocks; so no two are adjacent
STRIDE = 4


def cmd_ctx(dev):
    """A command context for a synchronous command on 'dev'"""

    ctx = xnvme.XnvmeCmdCtx()
    assert not xnvme.xnvme_dev_get_cmd_ctx(dev, ctypes.byref(ctx))
    return ctx


def check(err, ctx, name):
    """Assert that the command 'name' on 'ctx' returned 'err' 0 and succeeded"""

    status = xnvme.xnvme_cmd_ctx_cpl_status(ctypes.pointer(ctx))
    assert not (err or status), f"{name}(): err({err}), status(0x{status:x})"


def write(dev, nsid, slba, nlb, buf):
    ctx = cmd_ctx(dev)
    check(xnvme.xnvme_nvm_write(ctx, nsid, slba, nlb, buf, None), ctx, "write")


def read(dev, nsid, slba, nlb, buf):
    ctx = cmd_ctx(dev)
    check(xnvme.xnvme_nvm_read(ctx, nsid, slba, nlb, buf, None), ctx, "read")


@xnvme_parametrize(labels=["nvm"], opts=["be", "sync"])
def test_scattered_write_read(cijoe, device, be_opts, cli_args):
    """Write NBLOCKS blocks STRIDE apart, each its own content, and read them back"""

    dev = dev_from_params(device, be_opts)
    assert dev

    nsid = xnvme.xnvme_dev_get_nsid(dev)
    lba_nbytes = xnvme.xnvme_dev_get_geo(dev).contents.lba_nbytes
    nbytes = NBLOCKS * lba_nbytes

    wbuf, wview = buf_and_view(dev, nbytes)
    rbuf, rview = buf_and_view(dev, nbytes)
    wview[:] = numpy.arange(nbytes) % 251

    for i in range(NBLOCKS):
        write(dev, nsid, i * STRIDE, 0, wbuf + i * lba_nbytes)
    for i in range(NBLOCKS):
        read(dev, nsid, i * STRIDE, 0, rbuf + i * lba_nbytes)

    assert numpy.array_equal(rview, wview)

    xnvme.xnvme_buf_free(dev, rbuf)
    xnvme.xnvme_buf_free(dev, wbuf)
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_scopy(cijoe, device, be_opts, cli_args):
    """Copy blocks STRIDE apart, given as an array of source ranges in a buffer of
    the device, to consecutive blocks after them, and read those back"""

    if be_opts["admin"] == "block" or be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    dev = dev_from_params(device, be_opts)
    assert dev

    nsid = xnvme.xnvme_dev_get_nsid(dev)
    lba_nbytes = xnvme.xnvme_dev_get_geo(dev).contents.lba_nbytes
    ns = xnvme.xnvme_dev_get_ns(dev).contents
    nranges = min(NBLOCKS, ns.msrc + 1, ns.mcl)
    if not nranges:
        pytest.skip(reason="the namespace copies no blocks, MCL is zero")
    nbytes = nranges * lba_nbytes

    ranges_buf = xnvme.xnvme_buf_alloc(
        dev, ctypes.sizeof(xnvme.XnvmeSpecNvmScopySourceRange)
    )
    assert ranges_buf
    ctypes.memset(ranges_buf, 0, ctypes.sizeof(xnvme.XnvmeSpecNvmScopySourceRange))
    ranges = ctypes.cast(
        ranges_buf, ctypes.POINTER(xnvme.XnvmeSpecNvmScopySourceRange)
    ).contents
    for i in range(nranges):
        ranges.entry[i].slba = i * STRIDE
        ranges.entry[i].rpars = 0  # NLB, zero-based: one block
    sdlba = nranges * STRIDE

    dbuf, dview = buf_and_view(dev, nbytes)
    vbuf, vview = buf_and_view(dev, nbytes)
    dview[:] = numpy.arange(nbytes) % 251

    for i in range(nranges):
        write(dev, nsid, ranges.entry[i].slba, 0, dbuf + i * lba_nbytes)
    write(dev, nsid, sdlba, nranges - 1, vbuf)

    ctx = cmd_ctx(dev)
    err = xnvme.xnvme_nvm_scopy(
        ctx, nsid, sdlba, ranges.entry, nranges - 1, xnvme.XNVME_NVM_SCOPY_FMT_ZERO
    )
    check(err, ctx, "scopy")

    read(dev, nsid, sdlba, nranges - 1, vbuf)
    assert numpy.array_equal(vview, dview)

    xnvme.xnvme_buf_free(dev, vbuf)
    xnvme.xnvme_buf_free(dev, dbuf)
    xnvme.xnvme_buf_free(dev, ranges_buf)
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["write_zeroes"], opts=["be", "admin", "sync"])
def test_write_zeroes(cijoe, device, be_opts, cli_args):
    """Write NBLOCKS blocks, zero them with one Write Zeroes, and read zeroes back"""

    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: write_zeroes via {be_opts['sync']}")

    dev = dev_from_params(device, be_opts)
    assert dev

    nsid = xnvme.xnvme_dev_get_nsid(dev)
    lba_nbytes = xnvme.xnvme_dev_get_geo(dev).contents.lba_nbytes
    nbytes = NBLOCKS * lba_nbytes

    buf, view = buf_and_view(dev, nbytes)
    view[:] = ord("!")
    write(dev, nsid, 0, NBLOCKS - 1, buf)

    ctx = cmd_ctx(dev)
    check(xnvme.xnvme_nvm_write_zeroes(ctx, nsid, 0, NBLOCKS - 1), ctx, "write_zeroes")

    view[:] = ord("a")
    read(dev, nsid, 0, NBLOCKS - 1, buf)
    assert not view.any(), f"{numpy.count_nonzero(view)} bytes not zeroed"

    xnvme.xnvme_buf_free(dev, buf)
    xnvme.xnvme_dev_close(dev)
