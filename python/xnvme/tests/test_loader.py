import xnvme.ctypes_mapping as capi


def test_capi_is_loadable():
    """The 'capi' is loaded upon module access, this checks that it succeeded"""

    assert capi.is_loaded, "C library is not loaded; possibly missing library"


def test_capi_has_function():
    """
    This verifies that the loaded library actually is the xNVMe library, by checking for
    the existance of a xNVMe specific funtion.
    """

    assert capi.xnvme_enumerate, "Function-lookup failed, possibly incompatible library"


def test_capi_is_callable():
    """Verifies that at at least one function is callable"""

    assert capi.xnvme_libconf_pr(
        capi.XNVME_PR_DEF
    ), "Function-call failed; possibly missing or incompatible library"
