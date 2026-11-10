from ctypes import POINTER, byref, c_void_p, cast, pointer

import pytest
import xnvme
from conftest import xnvme_parametrize
from utils import dev_from_params, dev_open, pointer_equal


def test_version():
    assert xnvme.xnvme_ver_major() == 0, "Has major version been updated?"
    assert xnvme.xnvme_ver_minor() >= 6
    assert xnvme.xnvme_ver_patch() >= 0


def test_libconf():
    entries = xnvme.xnvme_libconf_get().contents.entries
    assert entries[0], "libconf has no entries"


def test_dev_open_uri_error():
    """Verify that a failed open returns a negative errno and leaves the handle NULL"""

    dev = POINTER(xnvme.XnvmeDev)()
    err = xnvme.xnvme_dev_open_uri("/nonexistent/xnvme", None, byref(dev))
    assert err < 0, "xnvme_dev_open_uri() of a path that does not exist succeeded"
    assert not dev, "xnvme_dev_open_uri() failed but set the handle"


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device without options"""

    if {"pcie", "fabrics"} & set(be_opts["label"]):
        pytest.skip("pcie and fabrics require nsid to be set in options.")

    dev = dev_open(device["uri"], None)
    assert (
        dev
    ), f"xnvme_dev_open_uri({device['uri']!r}, be={be_opts.get('be')}) returned NULL"
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open_with_default_opts(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device with default options"""

    if {"pcie", "fabrics"} & set(be_opts["label"]):
        pytest.skip("pcie and fabrics require nsid to be set in options.")

    opts = xnvme.XnvmeOpts()
    xnvme.xnvme_opts_set_defaults(byref(opts))

    dev = dev_open(device["uri"], byref(opts))
    assert (
        dev
    ), f"xnvme_dev_open_uri({device['uri']!r}, be={be_opts.get('be')}) returned NULL"
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_dev_open_with_opts(cijoe, device, be_opts, cli_args):
    """Verify that it is possible to open a device with default options"""

    dev = dev_from_params(device, be_opts)

    assert (
        dev
    ), f"xnvme_dev_open_uri({device['uri']!r}, be={be_opts.get('be')}) returned NULL"
    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_ident(cijoe, device, be_opts, cli_args):
    """Verify identifier information, atleast that nsid is as expected"""

    opts = xnvme.XnvmeOpts()
    xnvme.xnvme_opts_set_defaults(byref(opts))
    opts.nsid = device["nsid"]

    dev = dev_open(device["uri"], byref(opts))
    assert (
        dev
    ), f"xnvme_dev_open_uri({device['uri']!r}, be={be_opts.get('be')}) returned NULL"

    ident = xnvme.xnvme_dev_get_ident(dev)
    assert ident.contents.nsid == device["nsid"]

    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_pointers(cijoe, device, be_opts, cli_args):
    opts = xnvme.XnvmeOpts()
    xnvme.xnvme_opts_set_defaults(byref(opts))

    dev = dev_open(device["uri"], byref(opts))
    assert (
        dev
    ), f"xnvme_dev_open_uri({device['uri']!r}, be={be_opts.get('be')}) returned NULL"
    xnvme.xnvme_dev_close(dev)

    # A random xNVMe object which is easy to create
    ctx = xnvme.XnvmeCmdCtx()
    xnvme.xnvme_dev_get_cmd_ctx(dev, byref(ctx))
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
