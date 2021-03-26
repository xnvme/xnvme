.. _sec-backends-fbsd:

FreeBSD
=======

FreeBSD expose NVMe devices via the following device-files:

* ``/dev/nvme{ctrlr_id}``, NVMe Controller device node
* ``/dev/nvme{ctrlr_id}ns{ns_id}``, NVMe Namespace as device node
* ``/dev/nda{num}``, NVMe Namespace as storage device node
* ``/dev/nvd{num}``, NVMe Namespace as disk drive

For details see the following man-pages::

  nvme, nda, nvd, pci, disk, and nvmecontrol

The FreeBSD backend uses synchronous ``ioctl`` calls to have the kernel perform
NVMe commands. Note that the backend needs a re-implementation it is currently
not in a very usable state.

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_cmd_ctx <sec-c-api-xnvme-struct-xnvme_cmd_ctx>`.
