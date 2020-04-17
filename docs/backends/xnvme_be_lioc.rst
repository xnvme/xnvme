.. _sec-backends-lioc:

Linux/IOCTL
===========

The ``be:lioc`` backend communicates with the Linux NVMe driver via IOCTLs. The
backend relies on the driver-provided passthrough interface to submit admin,
IO, and arbitrary user-defined commands.

The backend does not support the asynchronous command interface, to enable it
for a subset of commands then two options are available:

* The :ref:`sec-backends-liou` backend (``be:liou``)
* The :ref:`sec-backends-laio` backend (``be:laio``)

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_req <sec-c-apis-xnvme-struct-xnvme_req>`.
