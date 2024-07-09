.. footer::

   ###Page### / ###Total###

.. header::

   ###Title### - ###Section###

====================================
 {{title}} - {{subtitle}}
====================================

.. contents:: Table of Contents
   :depth: 2

.. raw:: pdf

   PageBreak oneColumn

Audience
========

This report is of interest for those curious about:

* Storage Abstraction Layer overhead and encapsulation

  - Kernelspace storage interfaces
  - Userspace storage interfaces
  - And how these can be encapsulated with minimal overhead with **xNVMe**

* The xNVMe project

For the report a bit of background on **xNVMe** is needed, as such, 
this will be provided in the following section.

.. include:: xnvme.rst

Purpose
-------

With a storage abstraction layer such as **xNVMe** a natural question is: 

..

  What is the cost in **performance** for a system when using **xNVMe**
  as the storage abstraction layer?

The purpose of this report is to answer the above question. To do so, we first
qualify that by performance we will refer to the metric of latency (in nanoseconds)
on a single CPU. Lower latency means better performance.

In this report, we will compare the latency with and without using **xNVMe**
as the storage abstraction layer. In case **xNVMe** has a negative impact on 
performance, we wish to quantify this impact. 

.. raw:: pdf

   PageBreak


.. include:: testsetup.rst

.. raw:: pdf

   PageBreak


Methodology
===========

We run ``fio`` for a set of IO engines with varying IO depths and IO sizes. In each 
experiment, one of the two variables is kept constant while varying the other.
Specifically, the variables of the runs are:

.. list-table:: 
   :widths: 10 45 45
   :header-rows: 1

   * - Run
     - IO sizes
     - IO depths
   * - 1
     - 4096
     - 
       .. class:: tablebullet
       {% for depth in fio.iodepths %}
       * {{depth}}
       {% endfor %}
   * - 2
     - 
       .. class:: tablebullet
       {% for size in fio.iosizes %}
       * {{size}}
       {% endfor %}
     - 1

The purpose is to see how increasing IO depth and IO size affect the latency of the 
engine. 

For each (``io_engine``, (``io_size``, ``io_depth``)) in the product of IO engines and runs, 
where a run is a product of the IO sizes and IO depths in the table above, we run fio with 
the parameters below, where the ``engine_name`` is derived from the ``io_engine``.

{% block code %}
   ``fio 
   --name=<engine_name> 
   --filename=<device>
   --ioengine=<io_engine>
   --direct=1
   --rw=randread
   --size=1G
   --bs=<io_size>
   --iodepth=<io_depth>
   --output-format=json
   --time_based=1
   --group_reporting
   --runtime=10
   --eta-newline=1
   --ramp_time=5
   --norandommap=1
   --thread=1``
{% endblock %}

The engines used in this experiment are:

{% for engine in fio.ioengines %}
* {{engine.name}}
{% endfor %}


.. raw:: pdf

   PageBreak

Results
=======

.. raw:: pdf

{% for group, group_plots in plots.items() %}

.. raw:: pdf
   
   FrameBreak 300

{{group}}
---------

Below is the latency in nanoseconds with IO size 4096 and varying IO depths.

.. image:: {{ group_plots["iodepth"] }}
   :align: center
   :width: 70%

Below is the latency in nanoseconds with IO depth 1 and varying IO sizes.

.. image:: {{ group_plots["iosize"] }}
   :align: center
   :width: 70%

{% endfor %}

Latency at IO depth 1
========================

I/O latency when accessing the NVMe SSD with or without xNVMe.

.. image:: {{qd1_plot}}
   :align: center
   :width: 100%