from cython.operator import dereference

from libc.stdint cimport int8_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t
from libc.string cimport memset

import pytest
from conftest import BACKEND, DEVICE_NSID, DEVICE_PATH

cimport libxnvme


def test_dummy():
    assert True


def test_opts_struct():
    cdef libxnvme.xnvme_opts opts = libxnvme.xnvme_opts_default()
    opts.css.given = 0
    opts.css.value = 16
    cdef uint32_t* casted_css = <uint32_t *> &opts.css
    _css = dereference(casted_css)
    assert _css == 16

    opts.css.given = 1
    opts.css.value = 128
    cdef uint32_t* casted_css2 = <uint32_t *> &opts.css
    _css2 = dereference(casted_css2)
    assert _css2 == 1<<31 | 128

    # Try overrunning bitfield (won't work)
    opts.css.given = 0
    opts.css.value = 1<<31
    cdef uint32_t* casted_css3 = <uint32_t *> &opts.css
    _css3 = dereference(casted_css3)
    assert _css3 == 0


def test_dev_open_info():
    cdef libxnvme.xnvme_opts opts = libxnvme.xnvme_opts_default()
    opts.nsid = DEVICE_NSID
    opts.be = BACKEND
    cdef libxnvme.xnvme_dev* device = libxnvme.xnvme_dev_open(DEVICE_PATH, &opts)
    assert device, "Device didn't get opened"
    libxnvme.xnvme_dev_pr(device, libxnvme.XNVME_PR_DEF)
    libxnvme.xnvme_dev_close(device)

def test_dev_double_open():
    cdef libxnvme.xnvme_opts opts = libxnvme.xnvme_opts_default()
    opts.nsid = DEVICE_NSID
    opts.be = BACKEND
    cdef libxnvme.xnvme_dev* device = libxnvme.xnvme_dev_open(DEVICE_PATH, &opts)
    assert device, "Device didn't get opened"
    libxnvme.xnvme_dev_pr(device, libxnvme.XNVME_PR_DEF)
    libxnvme.xnvme_dev_close(device)

    # Open immediately again, testing a device detaching quickly
    device = libxnvme.xnvme_dev_open(DEVICE_PATH, &opts)
    assert device, "Device didn't get opened"
    libxnvme.xnvme_dev_pr(device, libxnvme.XNVME_PR_DEF)
    libxnvme.xnvme_dev_close(device)

# TODO: Disabled until fixed in SPDK. See test_enum_spdk_fail in Python bindings tests.

# cdef int xnvme_enumerate_callback_handler(libxnvme.xnvme_dev* dev, void* cb_args):
#     cdef int *enumerate_counter = <int*> cb_args
#     # Valid Cython dereferencing: https://cython.readthedocs.io/en/latest/src/userguide/language_basics.html#types
#     enumerate_counter[0] += 1
#     return libxnvme.XNVME_ENUMERATE_DEV_CLOSE

# def test_xnvme_enumerate():
#     cdef libxnvme.xnvme_opts opts2 = libxnvme.xnvme_opts_default()
#     opts2.nsid = DEVICE_NSID
#     opts2.be = BACKEND
#     cdef libxnvme.xnvme_dev* device = libxnvme.xnvme_dev_open(DEVICE_PATH, &opts2)
#     assert device, "Device didn't get opened"
#     libxnvme.xnvme_dev_close(device)

#     cdef int enumerate_counter = 0

#     cdef libxnvme.xnvme_opts opts
#     memset(&opts, 0, sizeof(libxnvme.xnvme_opts))

#     assert enumerate_counter == 0
#     libxnvme.xnvme_enumerate(NULL, &opts, xnvme_enumerate_callback_handler, &enumerate_counter)
#     assert enumerate_counter > 0, 'No devices enumerated'


# enumerate_counter = 0

# cdef int xnvme_enumerate_python_callback_handler(libxnvme.xnvme_dev* dev, void* cb_args):
#     global enumerate_counter
#     cdef object increments = <object> cb_args
#     enumerate_counter += increments
#     return libxnvme.XNVME_ENUMERATE_DEV_CLOSE

# def test_xnvme_enumerate_py_context():
#     cdef libxnvme.xnvme_opts opts2 = libxnvme.xnvme_opts_default()
#     opts2.nsid = DEVICE_NSID
#     opts2.be = BACKEND
#     cdef libxnvme.xnvme_dev* device = libxnvme.xnvme_dev_open(DEVICE_PATH, &opts2)
#     assert device, "Device didn't get opened"
#     libxnvme.xnvme_dev_pr(device, libxnvme.XNVME_PR_DEF)
#     libxnvme.xnvme_dev_close(device)

#     cdef object increments = 1
#     cdef void* cb_args_context = <void*>increments

#     cdef libxnvme.xnvme_opts opts
#     memset(&opts, 0, sizeof(libxnvme.xnvme_opts))

#     assert enumerate_counter == 0
#     libxnvme.xnvme_enumerate(NULL, &opts, xnvme_enumerate_python_callback_handler, cb_args_context)
#     assert enumerate_counter > 0, 'No devices enumerated'

