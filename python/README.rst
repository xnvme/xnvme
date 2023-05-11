====================================
 xNVMe language-bindings for Python
====================================

* bindings
  - Distribution package name: ``xnvme``
  - Provides Python bindings to **xNVMe** via ``ctypes``
  - Also, if needed then this package can ship plain-Python utilities
  - Intended to be distributed as a Python source-package

* tests
  - Tests implemented in Python using the xNVMe Python bindings

Usage notes
===========

The following is a summary of things that are nice to know when diving into
using the xNVMe Python bindings. In case you are familiar with Ctypes, then
most of this will be things you already know.

* When passing "strings", that is, things which in C are ``const char *str``.
  Then these needs to be encoded like so::

        ctypes.c_char_p("/dev/nvme0n1".encode("UTF-8")

Or in case you have a variable containing it for example::

        uri = "/dev/nvme0n1"
        dev = xnvme_dev_open(xnvme.cast_char_pointer(uri), None)

* When passing structs as pointer to a C function, then use
  ``ctypes.byref(mystruct)``

* When creating a pointer, then use ``ctypes.pointer``

* When declaring a pointer-type, the use ``ctypes.POINTER``

* Pointer-types evaluate to ``True`` when they point to an address and to
  ``False`` when pointing to ``NULL``
 
* Pass ``None`` where you in C would pass ``NULL``

For examples of the above, and general example of using the bindings then Have
a look at the contents of ``python/tests/test_*.py``, these pytest make use of
the xNVMe Python bindings.

Deprecated
==========

In version v0.6.0 of xNVMe multiple Python offerings were provided, these included:

* xnvme-core
  - Distribution package name: ``xnvme``
  - Provides Python bindings to **xNVMe** via ``ctypes``
  - Also, if needed then this package can ship plain-Python utilities
  - Intended to be distributed as a Python source-package

* xnvme-cy-header
  - distribution package name: ``xnvme-cy-header``
  - Provides a Cython (``libxnvme.pxd``) header, making the xNVMe C API
    available to Cython
  - Intended to be distributed as a Python source-package

* xnvme-cy-bindings
  - distribution package name: ``xnvme-cy-bindings``
  - Provides Python bindings to **xNVMe** via ``Cython``
  - Intended to be distributed as a Python binary wheel

These were intended as an initial exploration of which interface were a best
fit for xNVMe and now consolidated into a single ``ctypes`` based bindings.

.. _namespace packages: https://packaging.python.org/en/latest/guides/packaging-namespace-packages/
