======================================================
 xNVMe Cython and ctypes language-bindings for Python
======================================================

In an attempt to separate the build, runtime, and distrbution requirements of
the **xNVMe** Python bindings then they are packaged as `namespace packages`_.

* xnvme-core
  - Distribution package name: ``xnvme``
  - Provides Python bindings to **xNVMe** via ``ctypes``
  - Also, if needed then this package can ship plain-Python utilities
  - Intented to be distributed as a Python source-package

* xnvme-cython
  - distribution package name: ``xnvme-cython``
  - Provides Python bindings to **xNVMe** via ``Cython``
  - Intended to be distributed as a Python binary wheel

* xnvme-libraries
  - Distribution package name: ``xnvme-libraries``
  - If feasible, then this package should be distributed as a Python
    source-package containing the entire **xNVMe** source-archive, thus, upon
    install triggering the build of **xNVMe** producing the shared library

.. _namespace packages: https://packaging.python.org/en/latest/guides/packaging-namespace-packages/
