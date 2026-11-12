import ctypes

import xnvme


def test_cmd_ctx_cpl_status():
    """The status is non-zero when the Status Code or the Status Code Type is"""

    ctx = xnvme.XnvmeCmdCtx()
    assert xnvme.xnvme_cmd_ctx_cpl_status(ctypes.byref(ctx)) == 0

    ctx.cpl.status = 0x1  # the phase tag, which is not part of the status
    assert xnvme.xnvme_cmd_ctx_cpl_status(ctypes.byref(ctx)) == 0

    ctx.cpl.status = xnvme.XNVME_SPEC_CQE_STATUS_SC & 0x2
    assert xnvme.xnvme_cmd_ctx_cpl_status(ctypes.byref(ctx)) == 1

    ctx.cpl.status = xnvme.XNVME_SPEC_CQE_STATUS_SCT & 0x200
    assert xnvme.xnvme_cmd_ctx_cpl_status(ctypes.byref(ctx)) == 1


def test_cmd_ctx_data():
    """xnvme_cmd_ctx_get_data() returns the pointer given to xnvme_cmd_ctx_set_data()"""

    ctx = xnvme.XnvmeCmdCtx()
    assert not xnvme.xnvme_cmd_ctx_get_data(ctypes.byref(ctx))

    tag = ctypes.c_int(7)
    xnvme.xnvme_cmd_ctx_set_data(ctypes.byref(ctx), ctypes.addressof(tag))
    assert xnvme.xnvme_cmd_ctx_get_data(ctypes.byref(ctx)) == ctypes.addressof(tag)
