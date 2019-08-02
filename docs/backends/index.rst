.. _sec-backends:

==========
 Backends
==========

``xNVMe`` hides the implementation of operating system interaction from the
user. That is, the implementation of the ``xnvme_*`` **C API**, is delegated at
runtime to a backend implementation.

+--------------------+
| Backend Name       |
+====================+
| ``XNVME_BE_FIOC``  |
+--------------------+
| ``XNVME_BE_LIOC``  |
+--------------------+
| ``XNVME_BE_LIOU``  |
+--------------------+
| ``XNVME_BE_SPDK``  |
+--------------------+

By default ``xNVMe`` uses the device **URI** to determine which backend to use.
If it is a device path such as ``/dev/nvme0n1`` then the Linux backend is used,
when on Linux, when given device path on FreeBSD then the **XNVME_BE_FIOC** is
used.

If it is a pci device identifier, such as ``pci://0000:00:05.0/1``  then the
**SPDK** backend is used. The library can inform you which backend is in
affect, e.g.:

.. literalinclude:: xnvme_be_cli.cmd
   :language: bash

.. literalinclude:: xnvme_be_cli.out
   :language: bash

Not all backends support all features.

+----------------------------+-------------------------------------------+
|                            | Backends                                  |
+----------------------------+----------+----------+----------+----------+
| Feature                    | ``FIOC`` | ``LIOC`` | ``LIOU`` | ``SPDK`` |
+============================+==========+==========+==========+==========+
| Admin Commands             | **yes**  | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+
| I/O                        | **no**   | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+
| I/O *(w/ metadata)*        | **no**   | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+
| SGLs                       | **no**   | **no**   | **no**   | **yes**  |
+----------------------------+----------+----------+----------+----------+
| Async                      | **no**   | **no**   | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+

.. toctree::
   :hidden:

   xnvme_be_fioc
   xnvme_be_lioc
   xnvme_be_liou
   xnvme_be_spdk/index