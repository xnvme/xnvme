import ctypes
import errno

import pytest
import xnvme

DEPTH = 4


def open_ramdisk():
    opts = xnvme.XnvmeOpts()
    assert not xnvme.xnvme_opts_set_defaults(ctypes.byref(opts))
    opts.be = b"ramdisk"
    return xnvme.open_uri(b"1GB", ctypes.byref(opts))


def submit_read(dev, queue, buf):
    """Submit a read of the first LBA of 'dev' into 'buf' on 'queue'"""

    nbytes = dev.get_geo().lba_nbytes
    args = xnvme.XnvmeCmdPrepNvmReadArgs(
        args_nbytes=ctypes.sizeof(xnvme.XnvmeCmdPrepNvmReadArgs),
        nsid=dev.get_nsid(),
        slba=0,
        nlb=0,
    )
    ctx = queue.get_cmd_ctx()
    assert not xnvme.xnvme_cmd_prep_nvm_read(
        ctypes.byref(ctx.contents.cmd), ctypes.byref(args)
    )
    assert not xnvme.xnvme_cmd_pass(ctx, buf, nbytes, None, 0)


def test_reap_returns_as_many_command_contexts_as_it_stored():
    """Queue.reap() returns the command-contexts that 'ncompleted' counts, not
    all 'max' entries of the array"""

    with open_ramdisk() as dev:
        buf = dev.buf_alloc(dev.get_geo().lba_nbytes)
        with dev.queue_init(DEPTH, xnvme.XNVME_QUEUE_REAP) as queue:
            submit_read(dev, queue, buf)

            reaped = []
            while not reaped:
                reaped = queue.reap(DEPTH)

            assert len(reaped) == 1
            assert not xnvme.xnvme_cmd_ctx_cpl_status(reaped[0])
            queue.put_cmd_ctx(reaped[0])
        dev.buf_free(buf)


def test_reap_raises_with_the_command_contexts_it_stored():
    """The OSError of a failed Queue.reap() carries what it stored as 'partial',
    which is nothing for a queue initialized without XNVME_QUEUE_REAP"""

    with open_ramdisk() as dev:
        with dev.queue_init(DEPTH, 0) as queue:
            with pytest.raises(OSError) as raised:
                queue.reap(DEPTH)

    assert raised.value.errno == errno.EINVAL
    assert raised.value.partial == []
