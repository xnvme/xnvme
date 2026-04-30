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

This report is for evaluating the steady-state performance difference between
the ``uio_pci_generic`` and ``vfio-pci`` uPCIe paths in xNVMe.

.. include:: xnvme.rst

Purpose
========

The benchmark compares xNVMe ``upcie`` results collected from two separate boot
configurations:

* ``uio_pci_generic`` from a no-IOMMU setup
* ``vfio-pci`` from an IOMMU-enabled setup

The goal is to quantify the observed IOPS delta between the two paths for the
same workload matrix.

.. raw:: pdf

   PageBreak

.. include:: testsetup.rst

.. raw:: pdf

   PageBreak

Methodology
===========

Each input run executes ``xnvmeperf`` and ``fio`` against a single driver in the
current boot configuration. ``xnvmeperf`` provides the primary throughput result.
``fio`` provides an independent throughput result, the reported mean latency from
``lat_ns.mean``, and tail completion latency from ``clat_ns.percentile``. The
comparison step combines the normalized UIO and VFIO outputs by matching ``rw``,
``iosize``, and ``iodepth``.

``xnvmeperf`` receives the configured CPU mask directly. ``fio`` runs with
``numjobs`` set to the CPU mask popcount and ``cpus_allowed`` set to the expanded
CPU list.

Throughput deltas are reported per source using:

``(vfio_iops - uio_iops) / uio_iops * 100``

A positive IOPS delta means VFIO delivered higher throughput than UIO. A negative
IOPS delta means VFIO delivered lower throughput than UIO.

The latency plots and summary table present ``fio`` latency in microseconds for
readability. The latency delta is computed as ``(vfio - uio) / uio * 100``. A
positive latency delta means VFIO has higher measured latency. A negative latency
delta means VFIO has lower measured latency.

Tail latency uses the same delta convention: a positive tail latency delta means
VFIO has higher tail latency, while a negative tail latency delta means VFIO has
lower tail latency. Delta values are reported in the summary tables.

{% if workloads %}
Workload Matrix
---------------

.. list-table::
   :widths: 18 24 40
   :header-rows: 1

   * - RW
     - IO sizes
     - IO depths
{% for workload in workloads %}
   * - {{ workload.rw }}
     - {{ workload.iosizes }}
     - {{ workload.iodepths }}
{% endfor %}

{% endif %}

xnvmeperf Throughput
====================

{% for section in sections %}

{{ section.rw }} / {{ section.iosize }} bytes
-------------------------------

.. image:: {{ section.plots["iops"] }}
   :align: center
   :width: 85%

Delta: ``(vfio - uio) / uio * 100``. Positive means VFIO has higher IOPS;
negative means VFIO has lower IOPS.

.. list-table::
   :widths: 12 12 12 12
   :header-rows: 1

   * - IO depth
     - UIO IOPS
     - VFIO IOPS
     - Delta %
{% for row in section.rows %}
   * - {{ row.iodepth }}
     - {{ row.uio_iops }}
     - {{ row.vfio_iops }}
     - {{ row.delta_pct }}
{% endfor %}

{% endfor %}

fio Throughput
==============

{% for section in sections %}

{{ section.rw }} / {{ section.iosize }} bytes
-------------------------------

.. image:: {{ section.plots["fio_iops"] }}
   :align: center
   :width: 85%

Delta: ``(vfio - uio) / uio * 100``. Positive means VFIO has higher IOPS;
negative means VFIO has lower IOPS.

.. list-table::
   :widths: 12 12 12 12
   :header-rows: 1

   * - IO depth
     - UIO IOPS
     - VFIO IOPS
     - Delta %
{% for row in section.rows %}
   * - {{ row.iodepth }}
     - {{ row.uio_fio_iops }}
     - {{ row.vfio_fio_iops }}
     - {{ row.fio_delta_pct }}
{% endfor %}

{% endfor %}

Throughput Summary
==================

.. list-table::
   :widths: 10 10 10 12 12 12 12 10 10
   :header-rows: 1

   * - RW
     - IO size
     - IO depth
     - Source
     - UIO IOPS
     - VFIO IOPS
     - Delta %
     - UIO CV %
     - VFIO CV %
{% for row in rows %}
   * - {{ row.rw }}
     - {{ row.iosize }}
     - {{ row.iodepth }}
     - {{ row.throughput_runner }}
     - {{ row.uio_iops }}
     - {{ row.vfio_iops }}
     - {{ row.delta_pct }}
     - {{ row.uio_cv }}
     - {{ row.vfio_cv }}
   * - {{ row.rw }}
     - {{ row.iosize }}
     - {{ row.iodepth }}
     - {{ row.latency_runner }}
     - {{ row.uio_fio_iops }}
     - {{ row.vfio_fio_iops }}
     - {{ row.fio_delta_pct }}
     - {{ row.uio_fio_cv }}
     - {{ row.vfio_fio_cv }}
{% endfor %}

fio Latency
===========

{% for section in sections %}

{{ section.rw }} / {{ section.iosize }} bytes
-------------------------------

.. image:: {{ section.plots["latency"] }}
   :align: center
   :width: 85%

Delta: ``(vfio - uio) / uio * 100``. Positive means VFIO has higher latency;
negative means VFIO has lower latency.

.. list-table::
   :widths: 12 12 12 12
   :header-rows: 1

   * - IO depth
     - UIO us
     - VFIO us
     - Delta %
{% for row in section.rows %}
   * - {{ row.iodepth }}
     - {{ row.uio_lat_us }}
     - {{ row.vfio_lat_us }}
     - {{ row.lat_delta_pct }}
{% endfor %}

{% for percentile, label in tail_latencies %}

.. image:: {{ section.plots[percentile ~ "_latency"] }}
   :align: center
   :width: 85%

Delta: ``(vfio - uio) / uio * 100``. Positive means VFIO has higher {{ label }}
latency; negative means VFIO has lower {{ label }} latency.

.. list-table::
   :widths: 12 12 12 12
   :header-rows: 1

   * - IO depth
     - UIO us
     - VFIO us
     - Delta %
{% for row in section.rows %}
   * - {{ row.iodepth }}
     - {{ row["uio_" ~ percentile ~ "_us"] }}
     - {{ row["vfio_" ~ percentile ~ "_us"] }}
     - {{ row[percentile ~ "_delta_pct"] }}
{% endfor %}

{% endfor %}

{% endfor %}

Latency Summary
===============

.. list-table::
   :widths: 10 10 10 12 12 12 12
   :header-rows: 1

   * - RW
     - IO size
     - IO depth
     - Source
     - UIO FIO Latency (us)
     - VFIO FIO Latency (us)
     - Latency Delta %
{% for row in rows %}
   * - {{ row.rw }}
     - {{ row.iosize }}
     - {{ row.iodepth }}
     - {{ row.latency_runner }}
     - {{ row.uio_lat_us }}
     - {{ row.vfio_lat_us }}
     - {{ row.lat_delta_pct }}
{% endfor %}

Tail Latency Summary
====================

.. list-table::
   :widths: 9 9 9 10 10 10 10 10 10 10 10 10 10
   :header-rows: 1

   * - RW
     - IO size
     - IO depth
     - Source
     - UIO P99.9 us
     - VFIO P99.9 us
     - P99.9 Delta %
     - UIO P99.99 us
     - VFIO P99.99 us
     - P99.99 Delta %
     - UIO P99.999 us
     - VFIO P99.999 us
     - P99.999 Delta %
{% for row in rows %}
   * - {{ row.rw }}
     - {{ row.iosize }}
     - {{ row.iodepth }}
     - {{ row.latency_runner }}
     - {{ row.uio_p99_9_us }}
     - {{ row.vfio_p99_9_us }}
     - {{ row.p99_9_delta_pct }}
     - {{ row.uio_p99_99_us }}
     - {{ row.vfio_p99_99_us }}
     - {{ row.p99_99_delta_pct }}
     - {{ row.uio_p99_999_us }}
     - {{ row.vfio_p99_999_us }}
     - {{ row.p99_999_delta_pct }}
{% endfor %}
