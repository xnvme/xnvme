.. footer::

   ###Page### / ###Total###

.. header::

   ###Title### - ###Section###

====================================
 xNVMe in SPDK - Performance Report
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

SSD Preconditioning
-------------------

We are interested in examing relative performance of
storage abstraction layers, for this we are exclusively issuing read commands. No
write commands are sent to the devices. This is to avoid the effects of device-side
garbage collection, write buffer flushes etc.

Thus, currently, no preconditioning is done, since without issuing write commands, we
mitigate black box effects of device-side logic.

.. raw:: pdf

   PageBreak

Methodology
===========

Establish roofline:

* Maximum achievable IOPS rate possible with the given hardware.

  - Hardware SPECs 8M IOPS

* Maximum achievable IOPS rate of ``bdev_nvme``

  - Established by measuring rates obtained with ``bdevperf``

* Maximum achievable IOPS rate of ``io_uring`` and ``io_uring_cmd``

  - Established by measuring rates achieved with ``t/io_uring``

* Measure the IOPS rate of ``bdev_xnvme``, ``bdev_aio``, and ``bdev_uring``

With all the above, we can observe/compare how far the different IO
storage-paths are from the HW-roofline.

.. raw:: pdf

   PageBreak

Results
=======

.. raw:: pdf

   PageBreak

libaio
------

Comparing ``bdev_xnvme``, with ``io_mechanism=libaio``, to ``bdev_aio``.

The line plot below displays the relationship between iodepth on the x-axis and
IOPS (Input/Output Operations Per Second) on the y-axis, illustrating how IOPS
performance varies with different iodepth values.

.. image:: bdevperf_lineplot_libaio.png
   :align: center
   :width: 100%

For ``libaio``, as shown above, the two lines overlap closely on the plot,
indicating similar IOPS performance for ``bdev_xnvme`` with ``io_mechanism=libaio`` and ``bdev_aio`` across various iodepth
values

.. raw:: pdf

   PageBreak

io_uring
--------

Comparing ``bdev_xnvme``, using ``io_mechanism=io_uring``, to ``bdev_uring``.

The line plot below displays the relationship between iodepth on the x-axis and
IOPS (Input/Output Operations Per Second) on the y-axis, illustrating how IOPS
performance varies with different iodepth values.

.. image:: bdevperf_lineplot_io_uring.png
   :align: center
   :width: 100%

For ``io_uring``, as shown above, The plot reveals that **without** IO-polling
(``conserve_cpu=1``), then ``bdev_xnvme`` exhibits slightly lower IOPS than the
reference implementation. However, when IO-polling is enabled
(``conserve_cpu=0``), there's a noticeable boost in IOPS, showcasing its
distinct advantage.

io_uring_cmd
------------

For this, there is no reference bdev-implementation, thus, the graph stands alone.

The line plot below displays the relationship between iodepth on the x-axis and
IOPS (Input/Output Operations Per Second) on the y-axis, illustrating how IOPS
performance varies with different iodepth values.

.. image:: bdevperf_lineplot_io_uring_cmd.png
   :align: center
   :width: 100%

In the graph, the line representing io_uring_cmd shows a modest but clear
advantage over the other datasets, indicating that io_uring_cmd provides a
somewhat improved IOPS performance in this scenario.
