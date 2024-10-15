.. _sec-backends-macos:

macOS
=====

The macOS/DriverKit backend in xNVMe `requires MacVFN to be installed <https://github.com/samsungds/macvfn>`_.

This backend allows users of Thunderbolt NVMe devices to access them through
xNVMe. It supports both custom admin commands and I/O.

MacVFN currently has some limitations, as you'll need to disable System
Integrity Protection (SIP). The device will also be completely hooked by the
MacVFN driver and not appear as a mountable volume anymore.

To use with xNVMe, the fastest way to identify devices, is to run ``xnvme enum`` on the target
system. If succesful, it will present devices in the form of ``MacVFN-abcd``
, where the latter part is the serial number of the device.

This URI can be used in the xNVMe command line tools and C API::

  # Example of identify with MacVFN
  xnvme idfy --cns 1 MacVFN-abcd
