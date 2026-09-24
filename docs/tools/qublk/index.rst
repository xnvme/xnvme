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

``del`` — Delete a leftover device
==================================

A ``qublk run`` that is killed rather than signalled cleanly leaves its ublk
device behind. ``del`` stops and deletes such a device by identifier::

   qublk del --dev-id 0

.. literalinclude:: qublk_del_usage.out
   :language: bash
