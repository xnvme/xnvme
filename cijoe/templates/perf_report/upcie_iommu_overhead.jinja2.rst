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

Each input run executes ``xnvmeperf`` against a single driver in the current boot
configuration. The comparison step combines the normalized UIO and VFIO outputs
by matching ``rw``, ``iosize``, and ``iodepth``.

The reported delta is:

``(vfio_iops - uio_iops) / uio_iops * 100``

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

Results
=======

{% for key, group_plots in plots.items() %}

{{ key[0] }} / {{ key[1] }} bytes
-------------------------------

.. image:: {{ group_plots["iops"] }}
   :align: center
   :width: 85%

.. image:: {{ group_plots["delta"] }}
   :align: center
   :width: 85%

{% endfor %}

Summary
=======

.. list-table::
   :widths: 12 12 12 16 16 12 10 10
   :header-rows: 1

   * - RW
     - IO size
     - IO depth
     - UIO IOPS
     - VFIO IOPS
     - Delta %
     - UIO CV %
     - VFIO CV %
{% for row in rows %}
   * - {{ row.rw }}
     - {{ row.iosize }}
     - {{ row.iodepth }}
     - {{ row.uio_iops }}
     - {{ row.vfio_iops }}
     - {{ row.delta_pct }}
     - {{ row.uio_cv }}
     - {{ row.vfio_cv }}
{% endfor %}
