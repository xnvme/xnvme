.. _sec-ci-bench:

Bench
#####

.. figure:: ../../../_static/xnvme-ci-overview.png
   :alt: xNVMe CI Resource Overview
   :align: center

   xNVMe **CI** environments and resources

A custom physical setup is utilized to perform benchmarking of xNVMe. This
section presents the hardware in the **BENCH** part of the xNVMe CI environment.

Hardware
========

bench-intel
-----------

.. list-table:: Hardware Specs. for ``bench-intel``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel 12th Gen. Core i5-12600
     - 32 GB (2x Kingston 548C38-16)
     - MSI Z690-A
     - * 4x 980 PRO 2TB
       * 3x 980 PRO 1TB
       * 1x 980 PRO with Heatsink 1TB

bench-intel-pikvm
-----------------

This running :xref-sbc-pikvm:`PiKVM<>` on a **Raspberry Pi 4b**
with the :xref-sbc-pikvm-v3-hat:`PiKVM V3 HAT<>`, for setup notes
see :xref-sbc-pikvm-v3-hat-notes:`PiKVM V3 HAT Config Notes<>`.

perf-lat-fbsd
-------------

.. list-table:: Hardware Specs. for ``perf-lat-fbsd``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel Pentium Silver N6005 @ 2.00GHz
     - 16GB SODIMM Synchronous 2667 MT/s
     - HARDKERNEL ODROID-H3
     - - 1x MEMPEK1W016GA

perf-lat-linux
--------------

.. list-table:: Hardware Specs. for ``perf-lat-linux``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel Pentium Silver N6005 @ 2.00GHz
     - 16GB SODIMM Synchronous 2667 MT/s
     - HARDKERNEL ODROID-H3
     - - 1x MEMPEK1W016GA

perf-lat-win
------------

.. list-table:: Hardware Specs. for ``perf-lat-win``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel Pentium Silver N6005 @ 2.00GHz
     - 16GB SODIMM Synchronous 2667 MT/s
     - HARDKERNEL ODROID-H3
     - - 1x MEMPEK1W016GA