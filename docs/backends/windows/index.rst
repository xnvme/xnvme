.. _sec-backends-windows:

Windows
=======

The Windows backend communicates with the Windows NVMe driver StorNVMe.sys via
IOCTLs. The backend relies on the NVMe pass-through through IOCTLs to submit
admin, IO and arbitrary user-defined commands.

Device Identifiers
------------------

...


System Configuration
--------------------

Windows 10 or later version is currently preferred as it has all the features
which **xNVMe** utilizes. This section also gives you a brief overview of the
different I/O paths and APIs which the **xNVMe** API unifies access to.

Instrumentation
---------------

...


.. toctree::
   :maxdepth: 2
   :hidden:

   files.rst
   ioctl.rst
   iocp.rst
   iocp_th.rst
   io_ring.rst
