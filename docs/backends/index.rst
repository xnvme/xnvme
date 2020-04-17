.. _sec-backends:

==========
 Backends
==========

``xNVMe`` hides the implementation of operating system interaction from the
user. That is, the implementation of the ``xnvme_*`` **C API**, is delegated at
runtime to a backend implementation.

Devices are associated with a backend when they are "opened" via a device
identifier on the form::

  scheme:target[?option=val]

Such as::

  laio:/dev/nvme0n1
  liou:/dev/nvme0n1
  lioc:/dev/nvme0n1
  fioc:/dev/nvme0ns1
  pci:0000:01:00.0?nsid=1

If the ``scheme:`` part of the uri is not provided, then the first backend
capable of opening the given device does so. E.g. when providing only::

  /dev/nvme0n1

Then one of ``laio``, ``lioc``, and ``liou`` is associated. The scheme is
provided to deterministically associate a given backend. Additionally, the
uri-encoding is used to provide backend specific options.

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

+----------------------------+------------------------------------------------------+
|                            | Backends                                             |
+----------------------------+----------+----------+----------+----------+----------+
| Feature                    | ``FIOC`` | ``LIOC`` | ``LIOU`` | ``LAIO`` | ``SPDK`` |
+============================+==========+==========+==========+==========+==========+
| Admin Commands             | **yes**  | **yes**  | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+----------+
| I/O                        | **no**   | **yes**  | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+----------+
| I/O *(w/ metadata)*        | **no**   | **yes**  | **no**   | **no**   | **yes**  |
+----------------------------+----------+----------+----------+----------+----------+
| SGLs                       | **no**   | **no**   | **no**   | **no**   | **yes**  |
+----------------------------+----------+----------+----------+----------+----------+
| Async                      | **no**   | **no**   | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+----------+----------+

.. toctree::
   :hidden:

   xnvme_be_fioc
   xnvme_be_lioc
   xnvme_be_liou
   xnvme_be_laio
   xnvme_be_spdk/index
