.. footer::

   ###Page### / ###Total###

.. header::

   ###Title### - ###Section###

====================================
 xNVMe in SPDK - Latency Report
====================================

.. contents:: Table of Contents
   :depth: 2

.. raw:: pdf

   PageBreak oneColumn

Audience
========

This report is of interest for those curious about:

* Storage Abstraction Layer overhead and encapsulation

  - The efficient Linux interfaces (``io_uring`` and ``io_uring_cmd``)
  - The legacy Linux interfaces (``libaio``)
  - And how these can be encapsulated with minimal overhead with **xNVMe**

* The xNVMe project

  - The integration of xNVMe in SPDK via ``bdev_xnvme``
  - Efficiency of ``bdev_xnvme`` compared to ``bdev_uring`` / ``bdev_aio`` / ``bdev_nvme``

For the report a bit of background on **xNVMe** is needed, as such, this will be provided in the following section.

What is xNVMe?
--------------

**xNVMe** is a low-level storage abstraction layer. It is designed for
convenient use of NVMe devices, however, not limited to these. At its core
**xNVMe** provides an API operating on **commands**, that is, their
construction, subsmission, and completion. This can be in a synchronous / blocking
manner, or asynchronously / non-blocking via **queues**.

This **foundational** abstraction based on **commands**, encapsulates the
command-transport and semantics usually tied to storage interfaces.

That is, with **xNVMe** there is no assumption that the commands are
read/write commands.

By decomposing the storage interface in this manner, then any API such as the
blocking ``preadv/pwritev`` system calls, the asynchrous
``io_uring``/``io_uring_cmd`` system interface, or the rich **NVMe** commands
and IO queue pairs, can be represented.

And so they are. The API provided by **xNVMe** has implementations on top of:
``libaio``, ``POSIX aio``, ``io_uring``, ``io_uring_cmd``, **SPDK
NVMe driver**, **libvfn NVMe driver**, OS ioctl() interfaces, thread pools, and
async. emulation.

As such, **xNVMe** provides the encapsulation of the storage interfaces of
operating system provided abstractions, user space NVMe drivers, storage system
interfaces, and storage system APIs.

Purpose
-------

With a storage abstraction layer such as **xNVMe** a natural question is: 

..

  What is the cost in **performance** for a system when using **xNVMe**
  as the storage abstraction layer? 

The purpose of this report is to answer the above question. To do so, we first
qualify that by performance we will refer to the metric of IO Operations per
Second (IOPS) on a single saturated CPU.

Thus, in case **xNVMe** has negative impact on IOPS, then we wish to quantity
how many. And in general, then we wish to compare the effect on achieved IOPS
rate relatively.

That is, comparing the IOPS with and without using **xNVMe** as the
storage abstraction layer for the SPDK bdev abstraction.


.. raw:: pdf

   PageBreak


Test Setup
==========

.. note:: **TODO**: Write some prose here.

.. raw:: pdf

   PageBreak

Software
--------

.. note::
   **TODO**: Populate this section using the artifacts produces and data collected by
   **CIJOE**, that is, the output from ``cat /boot/cmdline``, ``uname``,
   ``dmesg``, ``/etc/os-release``.

.. raw:: pdf

   PageBreak

Hardware
--------

.. note::
   **TODO**: Populate this section using the artifacts produces by CIJOE, that is, the ``lshw``.

BIOS
----

.. note::
   **TODO**: Add BIOS name and version and describe settings changed from the default.

Methodology
===========



.. raw:: pdf

   PageBreak

Results
=======

.. raw:: pdf

{% for plot in plots %}

   PageBreak

{{plot.title}}
------

.. image:: {{plot.png}}
   :align: center
   :width: 100%

.. raw:: pdf

{% endfor %}