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

  /dev/nvme0n1
  file:/dev/nvme0n1
  file:/dev/nvme0ns1
  pci:0000:01:00.0?nsid=1

If the ``scheme:`` part of the uri is not provided, then the first backend
capable of opening the given device does so. E.g. when providing only::

  /dev/nvme0n1

Then, when on Linux, the Linux backend is associated, and when on FreeBSD the Freebsd backend is associated.
The uri-encoding is used to provide backend specific options.

If it is a pci device identifier, such as ``pci:0000:00:05.0?nsid=1``  then the
**SPDK** backend is used. The library can inform you which backend is in
affect, e.g.:

.. literalinclude:: xnvme_be_cli.cmd
   :language: bash

.. literalinclude:: xnvme_be_cli.out
   :language: bash

Not all backends support all features.

+----------------------------+--------------------------------+
|                            | Backends                       |
+----------------------------+----------+----------+----------+
| Feature                    | ``FBSD`` | ``LKRN`` | ``SPDK`` |
+============================+==========+==========+==========+
| Admin Commands             | **yes**  | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+
| I/O                        | **no**   | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+
| I/O *(w/ metadata)*        | **no**   | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+
| SGLs                       | **no**   | **no**   | **yes**  |
+----------------------------+----------+----------+----------+
| Async                      | **no**   | **yes**  | **yes**  |
+----------------------------+----------+----------+----------+

.. toctree::
   :hidden:

   xnvme_be_fbsd
   xnvme_be_linux
   xnvme_be_spdk/index
   xnvme_be_intf
