.. _sec-backends-linux-block:

Sync. I/O via ``block`` ioctl()
===============================

In case your device is **not** an NVMe device, then there is no NVMe-driver
ioctl() to make use of. **xNVMe** will then try to utilize the Linux Block Layer
and treat a given block device as a NVMe device via shim-layer for NVMe admin
commands such as identify and get-features.

Regular Block Devices
~~~~~~~~~~~~~~~~~~~~~

A brief example of checking this:

.. literalinclude:: xnvme_info_block.cmd
   :language: bash

Yielding:

.. literalinclude:: xnvme_info_block.out
   :language: bash


Zoned Block Devices
~~~~~~~~~~~~~~~~~~~

Building on the Linux Block model, then the Zoned Block Device model is also
utilized, specifically the following IOCTLs:

* ``BLK_ZONE_REP_CAPACITY``
* ``BLKCLOSEZONE``
* ``BLKFINISHZONE``
* ``BLKOPENZONE``
* ``BLKRESETZONE``
* ``BLKGETNRZONES``
* ``BLKREPORTZONE``

When available, then **xNVMe** can make use of the above IOCTLs. By doing
so, **xNVMe** provides a unified API to **Zoned** storage devices, with full
capabilities, regardless of whether the **zoned** storage device is **NVMe/
ZNS**, **SMR**, or some other form. And allows you to communicate efficiently
with your device.

.. literalinclude:: xnvme_info_zoned.cmd
   :language: bash

Yielding:

.. literalinclude:: xnvme_info_zoned.out
   :language: bash