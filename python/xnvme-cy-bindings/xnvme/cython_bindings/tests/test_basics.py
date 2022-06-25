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
