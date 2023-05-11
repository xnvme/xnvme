import xnvme.cython_bindings as xnvme


def test_version():
    # Very simple test that checks the bindings without an NVMe disk
    assert xnvme.xnvme_ver_major() == 0, "Has major version been updated?"
    assert xnvme.xnvme_ver_minor() >= 3
    assert xnvme.xnvme_ver_patch() >= 0


def test_libconf():
    fd = xnvme.FILE()
    fd.tmpfile()
    xnvme.xnvme_libconf_fpr(fd, xnvme.XNVME_PR_DEF)
    data = fd.getvalue()
    print(data.decode())
    assert len(data) != 0, "libconf file-stream is empty!"


def test_xnvme_ver_fpr():
    fd = xnvme.FILE()
    fd.tmpfile()
    xnvme.xnvme_ver_fpr(fd, xnvme.XNVME_PR_DEF)
    data = fd.getvalue()
    print(data.decode())
    assert len(data) != 0, "version file-stream is empty!"


def test_dev(dev):
    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)


def test_ident(dev):
    ident = xnvme.xnvme_dev_get_ident(dev)
    assert ident.nsid == 1
    assert ident.to_dict()


def test_pointers(dev):
    # A random xNVMe object which is easy to create
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)

    # Verify that we are pointing to the struct cmd embedded at the origin of ctx (they will share address)
    assert ctx.void_pointer == ctx.cmd.void_pointer

    # Verify that we are pointing to the struct cpl embedded at the origin of ctx + the size of cmd
    assert ctx.void_pointer + ctx.cmd.sizeof == ctx.cpl.void_pointer

    # Check pointers are well defined and stable -- i.e. not generated at each call
    assert ctx.dev.void_pointer == ctx.dev.void_pointer

    # Check that a pointer from two places to the same struct shows the same address
    assert ctx.dev.void_pointer == xnvme.xnvme_dev_get_geo(ctx.dev).void_pointer
