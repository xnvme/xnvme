"""
Utilities used with pytest / xnvme-python-bindings
"""
import ctypes

import numpy
import xnvme.ctypes_bindings as xnvme
from xnvme.ctypes_bindings.api import char_pointer_cast


def xnvme_cmd_ctx_cpl_status(ctx):
    """This function is a 'static inline' in the C API"""

    return ctx.contents.cpl.status.sc or ctx.contents.cpl.status.sct


def pointer_equal(ptr1, ptr2):
    """Check whether two pointer-types are equal, that is, they point to the same memory"""

    return (
        ctypes.cast(ptr1, ctypes.c_void_p).value
        == ctypes.cast(ptr2, ctypes.c_void_p).value
    )


def dev_from_params(device, be_opts):
    """
    Returns a device-handle as described by the given dictionary-parameters

    The structure of the given 'device' and 'be_opts' dicts are as defined by
    the pytest-fixture named xnvme_parametrize.
    """

    opts = xnvme.xnvme_opts()
    xnvme.xnvme_opts_set_defaults(ctypes.byref(opts))

    # Cast Python-strings to char-pointers and assign them to the xnvme_opts
    for key, val in ((k, v) for k, v in be_opts.items() if k not in ["label"]):
        setattr(opts, key, char_pointer_cast(val))

    # Directly assign the namespace-identifier value
    opts.nsid = device["nsid"]

    return xnvme.xnvme_dev_open(char_pointer_cast(device["uri"]), ctypes.byref(opts))


def buf_and_view(dev, buffer_size):
    """
    Returns a buffer of the given 'buffer_size' along with a numpy-view

    The view covers the entire buffer and typed as uint8. The buffer is
    zero-filled via the view. Thus, the pages backing the buffer should be
    allocated.
    """

    buf = xnvme.xnvme_buf_alloc(dev, buffer_size)

    view = numpy.ctypeslib.as_array(
        ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8)),
        shape=(buffer_size,),
    )
    view[:] = 0  # Zero memory and force page allocation

    return buf, view
