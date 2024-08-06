.. _sec-bdev-xnvme:

xNVMe in SPDK (``bdev_xnvme``)
==============================

The xNVMe bdev module issues I/O to the underlying NVMe devices through various I/O 
mechanisms such as libaio, io_uring, Asynchronous IOCTL using io_uring passthrough, 
POSIX aio, emulated aio etc.

The user needs to configure SPDK to include xNVMe support:

``configure --with-xnvme``

To create a xNVMe bdev with given filename, bdev name and I/O mechanism use the 
bdev_xnvme_create RPC.

``rpc.py bdev_xnvme_create /dev/ng0n1 bdev_ng0n1 io_uring_cmd``

The most important I/O mechanisms are:

- "libaio"
- "io_uring"
- "io_uring_cmd" (requires linux kernel v5.19 or newer)

To remove a xnvme bdev use the ``bdev_xnvme_delete`` RPC.

``rpc.py bdev_xnvme_delete bdev_ng0n1``

For more information on SPDK bdev refer to SPDK: https://spdk.io/doc/bdev.html

Feature comparison
------------------

.. list-table::
  :widths: 40 20 20 20
  :header-rows: 1

  * -
    - SPDK: ``bdev_nvme.c``
    - SPDK: ``bdev_xnvme.c``
    - xNVMe
  * - Flush
    - ✓
    - 
    - 
  * - Write
    - ✓
    - ✓
    - ✓
  * - Read
    - ✓
    - ✓
    - ✓
  * - Write Uncorrectable
    - 
    - 
    - ✓
  * - Compare
    - ✓
    - 
    - ✓
  * - Write Zeroes
    - ✓
    - ✓
    - ✓
  * - Dataset Management
    - ✓ (unmap)
    - 
    - ✓
  * - Verify
    - 
    - 
    - ✓
  * - Copy
    - ✓
    - 
    - ✓ (scopy)
  * - Compare and Write (fused command)
    - ✓
    - 
    - 
  * - Reset
    - ✓
    - 
    - ✓
  * - Zone Append
    - ✓
    - 
    - ✓
  * - Zone Management Receive
    - ✓ (Get Zone Info)
    - 
    - ✓
  * - Zone Management Send
    - ✓ (Zone Management)
    - 
    - ✓
  * - NVMe (passthru)
    - ✓
    - 
    - ✓
  * - Abort
    - ✓
    - 
    - 
  * - Hot Remove
    - ✓ (hotplug)
    - 
    - 
  * - Protection Information
    - ✓
    - 
    - ✓
