import xnvme


def test_library_is_loadable():
    """The library is loaded on the first call; this checks that it is found"""

    assert xnvme.xnvme_ver_major() >= 0


def test_library_is_xnvme():
    """The loaded library has an xNVMe function with the signature it is bound by"""

    conf = xnvme.xnvme_libconf_get()
    assert conf, "xnvme_libconf_get() returned NULL; possibly an incompatible library"


def test_records_print():
    """A record returned by the library prints itself"""

    assert xnvme.XnvmeLibconf.pr(xnvme.xnvme_libconf_get(), xnvme.XNVME_PR_DEF) > 0
