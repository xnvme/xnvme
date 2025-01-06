.. _sec-ci-verify:

Verify
######

.. figure:: ../../../_static/xnvme-ci-overview.png
   :alt: xNVMe CI Resource Overview
   :align: center

   xNVMe **CI** environments and resources

A custom physical setup is utilized to perform functional regression. This
section presents the hardware in the **VERIFY** part of the xNVMe CI
environment.

Hardware
========

verify-debian
-------------

.. list-table:: Hardware Specs. for ``verify-debian``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - AMD Ryzen Threadripper 1900X 8-Core
     - 32 GB (4x CMK32GX4M4B3200C16)
     - ASRock X399 Phantom Gaming 6
     - - 3x MEMPEK1W016GA

verify-debian-pikvm
-------------------

This running :xref-sbc-pikvm:`PiKVM<>` on a **Raspberry Pi 4b**
with the :xref-sbc-pikvm-v3-hat:`PiKVM V3 HAT<>`, for setup notes
see :xref-sbc-pikvm-v3-hat-notes:`PiKVM V3 HAT Config Notes<>`.

verify-macos
------------

.. list-table:: Hardware Specs. for ``verify-macos``
   :widths: 30 30 40
   :header-rows: 1

   * - CPU
     - Memory
     - NVMe Devices
   * - Apple M1
     - 16 GB
     - - 1x Intel Optane M10 16GB