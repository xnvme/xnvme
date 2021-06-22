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
NVMe commands.

NVMe Driver IOCTL
-----------------

The FreeBSD NVMe Driver IOCTL provides a rich interface for submitting commands
similar to that of the Linux NVMe Driver IOCTL. However, for completion-errors,
the FreeBSD NVMe Driver IOCTL provides the entire NVMe Completion result, where
the Linux IOCTL maps the completion-result to an ``errno`` value and returns
that instead.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on
:ref:`xnvme_cmd_ctx <sec-c-api-xnvme-struct-xnvme_cmd_ctx>`.
