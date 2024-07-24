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

For the report a bit of background on **xNVMe** is needed, as such, this will be provided 
in the following section.

.. include:: xnvme.rst

Purpose
-------

With a storage abstraction layer such as **xNVMe** a natural question is: 

..

  What is the cost in **performance** for a system when using **xNVMe**
  as the storage abstraction layer? 

The purpose of this report is to answer the above question. To do so, we first
qualify that by performance we will refer to the metric of IO Operations per
Second (IOPS) on a single saturated CPU. Higher IOPS means better 
performance.

In this report, we will compare the IOPS with and without using **xNVMe**
as the storage abstraction layer for the SPDK bdev abstraction. In case 
**xNVMe** has a negative impact on IOPS, then we wish to quantify
how much. 

.. raw:: pdf

   PageBreak


.. include:: testsetup.rst

.. raw:: pdf

   PageBreak
     

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

CPU usage
---------
We use the Linux ``perf`` tool to record the events of running
``bdevperf``. With ``perf report``, we find the CPU usage of the 
``bdev`` polling on the two cores. The usage is summed to gain the 
total CPU usage in percent.


.. raw:: pdf

   PageBreak

Results
=======

The plots, in this section, display the relationship between iodepth on the x-axis and
IOPS (Input/Output Operations Per Second) on the y-axis, illustrating how IOPS
performance varies with different iodepth values.

Additionally, there are plots displaying the relationship between iodepth and CPU usage.
These illustrate how resource utilization varies with different iodepth values.

.. raw:: pdf

   PageBreak

libaio
------

Comparing ``bdev_xnvme``, with ``io_mechanism=libaio``, to ``bdev_aio``.

.. image:: bdevperf_iops_barplot_libaio.png
   :align: center
   :width: 80%

For ``libaio``, as shown above, the two lines overlap closely on the plot,
indicating similar IOPS performance for ``bdev_xnvme`` with ``io_mechanism=libaio`` and 
``bdev_aio`` across various iodepth values.

CPU usage
~~~~~~~~~

.. image:: bdevperf_cpu_barplot_libaio.png
   :align: center
   :width: 70%

.. raw:: pdf

   PageBreak


io_uring
--------

Comparing ``bdev_xnvme``, using ``io_mechanism=io_uring``, to ``bdev_uring``.

.. image:: bdevperf_iops_barplot_io_uring.png
   :align: center
   :width: 80%

For ``io_uring``, as shown above, The plot reveals that **without** IO-polling
(``conserve_cpu=1``), then ``bdev_xnvme`` exhibits slightly lower IOPS than the
reference implementation. However, when IO-polling is enabled
(``conserve_cpu=0``), there's a noticeable boost in IOPS, showcasing its
distinct advantage.

CPU usage
~~~~~~~~~

.. image:: bdevperf_cpu_barplot_io_uring.png
   :align: center
   :width: 70%

.. raw:: pdf

   PageBreak


io_uring_cmd
------------

For ``io_uring_cmd``, there is no reference bdev-implementation, thus ``bdev_xnvme``, using ``io_mechanism=io_uring_cmd``, stands alone.


.. image:: bdevperf_iops_barplot_io_uring_cmd.png
   :align: center
   :width: 80%

Comparing this graph to the graphs for ``libaio`` and ``io_uring``, ``io_uring_cmd`` shows a modest but clear
advantage over the others, indicating that ``io_uring_cmd`` provides an improved IOPS performance in this scenario.

CPU usage
~~~~~~~~~

.. image:: bdevperf_cpu_barplot_io_uring_cmd.png
   :align: center
   :width: 70%

.. raw:: pdf

   PageBreak


Latency at IO depth 1
=====================

I/O latency when accessing the NVMe SSD.

.. image:: bdevperf_barplot_qd1.png
   :align: center
   :width: 100%

