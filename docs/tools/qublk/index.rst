.. _sec-tools-qublk:

qublk
#####

**qublk** is a ublk server backed by **xNVMe**. It creates a ``/dev/ublkbN``
block device, receives ``READ`` / ``WRITE`` / ``FLUSH`` requests from the Linux
``ublk_drv`` over ``io_uring``, and services them through the **xNVMe**
asynchronous command interface. Thus, any **xNVMe** backend, such as
:ref:`sec-backends-upcie` or **io_uring**, becomes usable as an ordinary block
device.

``REQ_FUA`` and ``REQ_PREFLUSH`` are supported, as are multiple ublk hardware
queues via ``--nqueues``.

.. literalinclude:: qublk_usage.out
   :language: bash

Requirements
============

**qublk** is Linux-only and is built only when ``liburing`` and
``<linux/ublk_cmd.h>`` are available. At runtime it requires:

* ``root`` privileges

* the ublk driver, loaded with ``modprobe ublk_drv``

* for user space backends such as **uPCIe**, a device bound to
  ``uio_pci_generic`` and hugepages configured; see :ref:`sec-backends-upcie`

The Linux-NVMe-driver ioctl() mimic (see below) additionally needs a kernel
new enough to carry ``NVME_IOCTL_IO64_CMD_VEC`` in ``<linux/nvme_ioctl.h>``
(5.17+) alongside ``<linux/fuse.h>``, both at build time;
``-Dwith-cuse=disabled`` skips it explicitly, and without either header
**qublk** still builds, quietly serving every device without its ``-nvme``
character device, since that is a permanent build-time condition rather
than a one-off failure; a CUSE device that fails to come up at runtime for
some other reason still warns and does the same.

``run`` — Serve a block-device
==============================

Opens the given device URIs, adds a ublk device for each, and serves them
until ``SIGINT`` / ``SIGTERM``, upon which it performs a clean teardown
(``STOP_DEV`` followed by ``DEL_DEV``).

When ``--dev-id`` is not given, the kernel assigns the device identifiers;
when given, the devices are numbered consecutively from it.
When ``--max-io-bytes`` is not given, the per-IO buffer size defaults to the
smaller of 1MiB and the controller ``MDTS``.

By default, each queue is served by a thread of its own. With ``--cpumask`` or
``--cpulist``, a thread is pinned to each given CPU and the queues are spread
evenly across them.

.. literalinclude:: qublk_run_usage.out
   :language: bash

Example — NVMe block device via io_uring::

   qublk run /dev/nvme0n1 --be io_uring --qdepth 64

Example — user space NVMe via uPCIe, with multiple hardware queues::

   qublk run 0000:01:00.0 --be upcie --qdepth 64 --nqueues 4

Example — four devices via uPCIe, served by two CPUs::

   qublk run 0000:01:00.0 0000:02:00.0 0000:03:00.0 0000:04:00.0 \
     --be upcie --qdepth 64 --cpulist 0-1

While **qublk** is running, each device appears as ``/dev/ublkb<N>``.

A device driven by one of xNVMe's user-space NVMe drivers (``--be spdk``,
``libvfn`` or ``upcie``) also gets a ``/dev/ublkb<N>-nvme`` character device
answering the Linux kernel NVMe driver's ioctl() interface (``NVME_IOCTL_ID``,
``NVME_IOCTL_ADMIN_CMD``, ``NVME_IOCTL_IO_CMD``, ``NVME_IOCTL_ADMIN64_CMD``,
``NVME_IOCTL_IO64_CMD``, ``NVME_IOCTL_IO64_CMD_VEC``), so tools such as
``nvme-cli`` address it like a kernel-attached namespace. Other devices get
none: a kernel-managed NVMe namespace already has ``/dev/nvme<X>n<Y>`` and
``/dev/ng<X>n<Y>``, and a device that is not NVMe, such as ``/dev/sda`` or a
file, has no NVMe interface to mimic. Pass ``--no-cuse`` to skip it; a failed
CUSE device only disables itself, the block device still comes up.

Passthru data and metadata, together with the
``nvme_passthru_cmd``/``nvme_passthru_cmd64`` struct itself, are capped at
128KiB by the kernel's CUSE ioctl handling, not the device's real
``MDTS``. ``NVME_IOCTL_IO64_CMD_VEC`` additionally rejects a request whose
``vec_cnt`` is above 125: every retry round resends the same iovec list as
both the fetch and the scatter-back list, and the kernel's own
``FUSE_IOCTL_MAX_IOV`` (256) bounds their combined count, well below the
real driver's ``UIO_MAXIOV``.

``del`` — Delete a leftover device
==================================

A ``qublk run`` that is killed rather than signalled cleanly leaves its ublk
device behind. ``del`` stops and deletes such a device by identifier::

   qublk del --dev-id 0

.. literalinclude:: qublk_del_usage.out
   :language: bash
