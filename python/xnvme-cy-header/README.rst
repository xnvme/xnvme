=========================
 xNVMe Python interfaces
=========================

**xNVMe** provides three ways to consume the **xNVMe** C API from Python:

#. Python bindings via ctypes

   * Package Name: ``xnvme``

#. A Cython Header

   * Package name: ``xnvme-cy-header``

#. Python bindings via Cython

   * Package name: ``xnvme-cy-bindings``

This package is the second of three offererings. That is, the
``xnvme-cy-header`` package. Providing the Cython Header ``libxnvme.pxd`` for
the xNVMe C API.

For additional information see:

* The online docs_ for the latest released version
* The ``docs/python`` folder on the next_ branch of the repository_ for the
  upcoming release
* The ``docs/python/`` folder in the repository_ on any outstanding pull-requests_.

.. _docs: https://xnvme.io/docs/latest/python
.. _next: https://github.com/OpenMPDK/xNVMe/tree/next
.. _repository: https://github.com/OpenMPDK/xNVMe
.. _pull-requests: https://github.com/OpenMPDK/xNVMe/pulls
