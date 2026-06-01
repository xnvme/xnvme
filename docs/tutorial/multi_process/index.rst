.. _sec-tutorials-multi-process:

================
 Multi-Process
================

**xNVMe** supports multiple processes accessing the same userspace NVMe
device simultaneously. The primary process allocates all hardware queue pairs
upfront; each process then claims one or more pairs exclusively for I/O.
No queue pair is used by more than one process at a time.

Roles
=====

Each process that opens a device declares one of three roles via
``xnvme_opts.proc_role``:

``XNVME_PROC_SINGLE``
  Default. Single-process mode with no multi-process coordination. Queue pairs
  are allocated on demand; no shared memory is created. Use this when only one
  process will ever access the device.

``XNVME_PROC_PRIMARY``
  Initializes the hardware, pre-allocates the shared queue pool, and publishes
  shared state. The primary must open the device before any secondary process
  attempts to attach.

``XNVME_PROC_SECONDARY``
  Attaches to an already-initialized controller without touching the hardware.
  Returns an error if no primary is running for the given device.

Backends
========

Multi-process works across all backends. The difference is in how coordination
happens:

**Kernel-based backends** (e.g., ``linux``, ``io_uring``)
  The OS handles concurrent device access natively. Multiple processes can
  open the same device independently and ``proc_role`` has no effect.

**vfio** (``libvfn``)
  Single-process only. The VFIO group-based API allows a group to be in only
  one IOMMU container at a time, so a second process cannot share the
  primary's IOMMU domain. ``proc_role`` has no effect for this backend.

**upcie** (``uio_pci_generic`` driver only)
  Full multi-process support. The uPCIe backend programs the NVMe controller
  directly and coordinates through POSIX shared memory. Setting ``proc_role``
  tells the library which process should initialize the hardware and which
  should attach to an already-running controller. Without this coordination,
  a secondary process would reset the device and destroy in-flight I/O.

  Devices bound to ``vfio-pci`` do not support secondary attachment;
  ``XNVME_PROC_SECONDARY`` returns ``ENOTSUP`` for those devices.

Usage
=====

The queue API is identical regardless of process role. Each process calls
``xnvme_queue_init()`` to claim a queue pair and ``xnvme_queue_term()`` to
release it. Queue pairs are drawn from a pool pre-allocated by the primary at
device-open time.

Primary process
---------------

.. code-block:: c

   struct xnvme_opts opts = xnvme_opts_default();
   opts.be = "upcie";
   opts.proc_role = XNVME_PROC_PRIMARY;

   struct xnvme_dev *dev = xnvme_dev_open("0000:01:00.0", &opts);
   if (!dev) {
       perror("xnvme_dev_open");
       return 1;
   }

   struct xnvme_queue *queue;
   if (xnvme_queue_init(dev, 256, 0, &queue)) {
       perror("xnvme_queue_init");
       return 1;
   }

   /* submit and poll I/O as normal */

   xnvme_queue_term(queue);
   xnvme_dev_close(dev);

Secondary process
-----------------

.. code-block:: c

   struct xnvme_opts opts = xnvme_opts_default();
   opts.be = "upcie";
   opts.proc_role = XNVME_PROC_SECONDARY;

   struct xnvme_dev *dev = xnvme_dev_open("0000:01:00.0", &opts);
   if (!dev) {
       perror("xnvme_dev_open");
       return 1;
   }

   struct xnvme_queue *queue;
   if (xnvme_queue_init(dev, 256, 0, &queue)) {
       perror("xnvme_queue_init");
       return 1;
   }

   /* submit and poll I/O as normal */

   xnvme_queue_term(queue);
   xnvme_dev_close(dev);

Note that the only difference between the two snippets is ``proc_role``. Once
the device is open, all queue operations work identically.

Each process may open the same device multiple times (for example, once per
namespace) and init as many queues as needed, subject to the pool capacity
described below.

Queue pool
==========

The primary pre-allocates a fixed pool of NVMe I/O queue pairs at device-open
time. Secondary processes claim pairs from this pool atomically; no admin
commands are issued after primary initialization. The pool is released when
the primary calls ``xnvme_dev_close()``.

The pool holds at most 63 queue pairs per device. One of those pairs is
reserved for the synchronous command path used internally by each process, so
in practice each process that calls ``xnvme_dev_open()`` consumes one slot
before any ``xnvme_queue_init()`` calls. This leaves at most 62 slots
distributed across all attached processes.

Device geometry
===============

Secondary processes can call ``xnvme_dev_get_geo()``, ``xnvme_dev_get_ctrlr()``,
and related functions. The primary publishes its Identify data into shared
memory the first time any of those functions is called, and secondaries read
from shared memory instead of issuing admin commands.

This means the primary must trigger at least one identify-dependent call (e.g.
``xnvme_dev_get_geo()``) **before** any secondary calls those same functions.
In practice this happens naturally: the primary warms up the device and starts
I/O before launching secondary processes.

Only the namespace that the primary opened first is published. Secondaries must
open the same namespace ID as the primary. Attempting to derive geometry for a
different namespace from a secondary returns an error.

FS-interface I/O (byte-addressed reads and writes via ``XNVME_SPEC_FS_OPC_*``
opcodes) works correctly on secondaries once geometry is available.

Constraints
===========

* The primary must open the device before any secondary attempts to attach.
  A secondary that finds no primary returns ``ENOENT``.

* Secondaries must open the same namespace ID as the primary used for its
  first identify call.

* Each process is responsible for calling ``xnvme_queue_term()`` for every
  queue it has initialized and ``xnvme_dev_close()`` for every device it has
  opened, in order to return queue pairs to the pool.

* Recovery from a crashed primary is the caller's responsibility. If the
  primary exits without calling ``xnvme_dev_close()``, the hardware queue
  state is lost and attached secondaries are left in an undefined condition.
  The safest recovery path is to restart all processes.

* Buffers allocated with ``xnvme_buf_alloc()`` are per-process and must not
  be passed between processes.
