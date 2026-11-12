"""
Utilities used with pytest / xnvme-python-bindings
"""

import ctypes

import numpy
import xnvme


def pointer_equal(ptr1, ptr2):
    """Check whether two pointer-types are equal, that is, they point to the same memory"""

    return (
        ctypes.cast(ptr1, ctypes.c_void_p).value
        == ctypes.cast(ptr2, ctypes.c_void_p).value
    )


def dev_open(uri, opts):
    """The device xnvme_dev_open_uri() opens 'uri' with 'opts' as, or NULL"""

    dev = ctypes.POINTER(xnvme.XnvmeDev)()
    xnvme.xnvme_dev_open_uri(uri, opts, ctypes.byref(dev))
    return dev


def dev_from_params(device, be_opts):
    """
    Returns a device-handle as described by the given dictionary-parameters

    The structure of the given 'device' and 'be_opts' dicts are as defined by
    the pytest-fixture named xnvme_parametrize.
    """

    opts = xnvme.XnvmeOpts()
    xnvme.xnvme_opts_set_defaults(ctypes.byref(opts))

    # The string options are char arrays of xnvme_opts, which take bytes
    for key, val in ((k, v) for k, v in be_opts.items() if k not in ["label"]):
        setattr(opts, key, val.encode())

    # Directly assign the namespace-identifier value
    opts.nsid = device["nsid"]

    return dev_open(device["uri"], ctypes.byref(opts))


def args_of(record, **fields):
    """A record of the arguments of a command prep, its size set, and 'fields'"""

    return record(args_nbytes=ctypes.sizeof(record), **fields)


def prep_and_pass(ctx, prep, args, dbuf=None, dbuf_nbytes=0):
    """Prepare the command of 'ctx' with 'prep' from 'args', then pass it with
    'dbuf'; returns what the first of the two that fails returns, or 0"""

    err = prep(ctypes.byref(ctx.cmd), ctypes.byref(args))
    if err:
        return err

    return xnvme.xnvme_cmd_pass(ctypes.byref(ctx), dbuf, dbuf_nbytes, None, 0)


def buf_and_view(dev, buffer_size):
    """
    Returns a buffer of the given 'buffer_size' along with a numpy-view

    The view covers the entire buffer and typed as uint8. The buffer is
    zero-filled via the view. Thus, the pages backing the buffer should be
    allocated.
    """

    buf = xnvme.xnvme_buf_alloc(dev, buffer_size)
    assert buf, f"xnvme_buf_alloc() of {buffer_size} bytes failed"

    view = numpy.ctypeslib.as_array(
        ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8)),
        shape=(buffer_size,),
    )
    view[:] = 0  # Zero memory and force page allocation

    return buf, view
