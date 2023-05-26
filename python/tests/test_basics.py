from ctypes import byref, c_void_p, cast, pointer

import pytest
import xnvme.ctypes_bindings.api as xnvme
from conftest import xnvme_parametrize
from utils import dev_from_params, pointer_equal
from xnvme.ctypes_bindings.api import char_pointer_cast


def test_version():
    assert xnvme.xnvme_ver_major() == 0, "Has major version been updated?"
    assert xnvme.xnvme_ver_minor() >= 6
    assert xnvme.xnvme_ver_patch() >= 0


def test_xnvme_ver_fpr():
    count = xnvme.xnvme_ver_pr(xnvme.XNVME_PR_DEF)
    assert count, "version file-stream is empty!"


def test_libconf():
    count = xnvme.xnvme_libconf_pr(xnvme.XNVME_PR_DEF)
    assert count, "libconf file-stream is empty!"


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device without options"""

    if set(["pcie", "fabrics"]) and set(be_opts["label"]):
        pytest.skip("pcie and fabrics require nsid to be set in options.")

    dev = xnvme.xnvme_dev_open(char_pointer_cast(device["uri"]), None)
    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open_with_default_opts(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device with default options"""

    if set(["pcie", "fabrics"]) and set(be_opts["label"]):
        pytest.skip("pcie and fabrics require nsid to be set in options.")

    opts = xnvme.xnvme_opts()
    xnvme.xnvme_opts_set_defaults(byref(opts))

    dev = xnvme.xnvme_dev_open(char_pointer_cast(device["uri"]), byref(opts))
    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open_with_opts(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device with default options"""

    dev = dev_from_params(device, be_opts)

    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_ident(cijoe, device, be_opts, cli_args):
    """Verify identifier information, atleast that nsid is as expected"""

    opts = xnvme.xnvme_opts()
    xnvme.xnvme_opts_set_defaults(byref(opts))
    opts.nsid = device["nsid"]

    dev = xnvme.xnvme_dev_open(char_pointer_cast(device["uri"]), byref(opts))
    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)
    xnvme.xnvme_dev_close(dev)

    ident = xnvme.xnvme_dev_get_ident(dev)
    assert ident.contents.nsid == device["nsid"]


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_pointers(cijoe, device, be_opts, cli_args):
    opts = xnvme.xnvme_opts()
    xnvme.xnvme_opts_set_defaults(byref(opts))

    dev = xnvme.xnvme_dev_open(char_pointer_cast(device["uri"]), byref(opts))
    assert dev, xnvme.xnvme_dev_pr(dev, xnvme.XNVME_PR_DEF)
    xnvme.xnvme_dev_close(dev)

    # A random xNVMe object which is easy to create
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    ptr_a = pointer(ctx)
    ptr_b = pointer(ctx)

    # Verify that we are pointing to the struct cmd embedded at the origin of ctx (they will share address)
    assert pointer_equal(ptr_a, ptr_b)

    # Verify that we are pointing to the struct cpl embedded at the origin of ctx + the size of cmd
    # DROP: this is invalid, the entities are not pointer-type but value-types,
    # how this passed with Cython is beyond my understanding. Regardless, the
    # behavior is unwanted.

    # Check pointers are well defined and stable -- i.e. not generated at each call
    assert pointer_equal(ctx.dev, ctx.dev)

    # Check that a pointer from two places to the same struct shows the same address
    assert pointer_equal(ctx.dev, dev)
