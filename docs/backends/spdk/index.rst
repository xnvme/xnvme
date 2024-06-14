.. _sec-backends-spdk:

SPDK
====

**xNVMe** provides a kernel-bypassing backend implemented
using :xref-spdk:`SPDK<>`. **SPDK** has a lot to offer, however, **xNVMe**
only makes use of the :xref-spdk:`SPDK<>` user-space NVMe driver. That is, the
reactor, threading-model, and application-framework of **SPDK** is not used
by **xNVMe**.

.. _sec-backends-spdk-identifiers:

Device Identifiers
------------------

When using user-space NVMe-drivers, such as the SPDK NVMe PMD
(poll-mode-driver), then the operating-system kernel NVMe driver is "detached"
and the device bound to ``vfio-pci``` or ``uio-generic```. Thus, the
device-files in ``/ dev/``, such as ``/dev/nvme0n1`` are not available. Devices
are instead identified by their PCI id (``0000:03:00.0```), and namespace
identifier.

This information is retrievable via ``xnvme enum``:

.. literalinclude:: 400_xnvme_enum.cmd
   :language: bash

.. literalinclude:: 400_xnvme_enum.out
   :language: bash

This information is usable via the the cli, such as here:


.. literalinclude:: 115_xnvme_info.cmd
   :language: bash

And using the API it would be similar to::

  ...
  struct xnvme_dev *dev = xnvme_dev_open("0000:03:00.0", opts);
  ...


Notice that multiple URIs using the same PCI id but with different **xNVMe**
``?opts=<val>``. This is provided as a means to tell **xNVMe** that you want to
use the NVMe controller at ``0000:03:00.0`` and the namespace identified by
``nsid=1``.

.. literalinclude:: 410_xnvme_info.cmd
   :language: bash

.. literalinclude:: 410_xnvme_info.out
   :language: bash


.. _sec-backends-spdk-instrumentation:

Instrumentation
---------------

...

.. _sec-backends-spdk-config:

System Configuration
--------------------

...

Driver Attachment and memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, then it is the operating system responsibility to provide device
drivers, thus by default, then the operating system has bound its NVMe driver
to the NVMe devices attached to your system. For a user-space driver to operate,
then the operating system driver must be detached and bound to a driver such as
``vfio-pci`` or ``uio-generic-pci``, such that the NVMe driver can operate in
user-space.

By running the command below **8GB** of hugepages will be configured and the
device detached from the Kernel NVMe driver:

.. literalinclude:: 010_xnvme_driver.cmd
   :language: bash

The ``xnvme-driver`` script is a merge of the **SPDK** ``setup.sh`` script and
its dependencies.

The command above should produce output similar to:

.. literalinclude:: 010_xnvme_driver.out
   :language: bash

If anything else that the above is output from ``setup.sh``, for example::

  0000:01:00.0 (1d1d 2807): nvme -> uio_generic

Or::

  Current user memlock limit: 16 MB

  This is the maximum amount of memory you will be
  able to use with DPDK and VFIO if run as current user.
  To change this, please adjust limits.conf memlock limit for current user.

  ## WARNING: memlock limit is less than 64MB
  ## DPDK with VFIO may not be able to initialize if run as current user.

Then consult the section :ref:`sec-backends-spdk-vfio`.

Memory Issues
~~~~~~~~~~~~~

If you see a message similar to the below while unbinding devices::

  Current user memlock limit: 16 MB

  This is the maximum amount of memory you will be
  able to use with DPDK and VFIO if run as current user.
  To change this, please adjust limits.conf memlock limit for current user.

  ## WARNING: memlock limit is less than 64MB
  ## DPDK with VFIO may not be able to initialize if run as current user.

Then go you should do as suggested, that is, adjust ``limits.conf``, for an
example, see :ref:`sec-backends-spdk-config`.

Re-binding devices
~~~~~~~~~~~~~~~~~~

Run the following:

.. literalinclude:: 120_xnvme_driver_reset.cmd
   :language: bash

Should output similar to:

.. literalinclude:: 120_xnvme_driver_reset.out
   :language: bash

.. _sec-backends-spdk-vfio:

Enabling ``VFIO`` without limits
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If ``nvme`` is rebound to ``uio_generic``, and not ``vfio``, then VT-d is
probably not supported or disabled. In either case try these two steps:

1) Verify that your CPU supports VT-d and that it is enabled in BIOS.

2) Enable your kernel by providing the kernel option `intel_iommu=on`.  If you
   have a non-Intel CPU then consult documentation on enabling VT-d / IOMMU for
   your CPU.

3) Increase limits, open ``/etc/security/limits.conf`` and add:

::

  *    soft memlock unlimited
  *    hard memlock unlimited
  root soft memlock unlimited
  root hard memlock unlimited

Once you have gone through these steps, then this command:

.. literalinclude:: 200_dmesg.cmd
   :language: bash

Should output:

.. literalinclude:: 200_dmesg.out
   :language: bash

And this this command:

.. literalinclude:: 300_find.cmd
   :language: bash

Should have output similar to:

.. literalinclude:: 300_find.out
   :language: bash

And user-space driver setup:

.. literalinclude:: 400_xnvme_driver.cmd
   :language: bash

Should rebind the device to ``vfio-pci``, eg.:

.. literalinclude:: 400_xnvme_driver.out
   :language: bash

Inspecting and manually changing memory available to ``SPDK`` aka ``HUGEPAGES``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `SPDK` setup script provides `HUGEMEM` and `NRHUGE` environment variables
to control the amount of memory available via `HUGEPAGES`. However, if you want
to manually change or just inspect the `HUGEPAGE` config the have a look below.

Inspect the system configuration by running::

  grep . /sys/devices/system/node/node0/hugepages/hugepages-2048kB/*

If you have not yet run the setup script, then it will most likely output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

And after running the setup script it should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

This tells that `1024` hugepages, each of size `2048kB` are available, that is,
a total of two gigabytes can be used.

One way of increasing memory available to `SPDK` is by increasing the number of
`2048Kb` hugepages. E.g. increase from two to eight gigabytes by increasing
`nr_hugespages` to `4096`::

  echo "4096" > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

After doing this, then inspecting the configuration should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0


No devices found
~~~~~~~~~~~~~~~~

When running ``xnvme enum`` and the output-listing is empty, then there are no
devices. When running with ``vfio-pci``, this can occur when your devices are
sharing iommu-group with other devices which are still bound to in-kernel
drivers. This could be NICs, GPUs or other kinds of peripherals.

The division of devices into groups is not something that can be easily
switched, but you can try to manually unbind the other devices in the iommu
group from their kernel drivers.

If that is not an option then you can try to re-organize your physical
connectivity of deviecs, e.g. move devices around.

Lastly you can try using ``uio_pci_generic`` instead, this can most easily be
done by disabling iommu by adding the kernel option: ``iommu=off`` to the
kernel command-line and rebooting.

.. _sec-backends-spdk-config-userspace:

User Space
~~~~~~~~~~

Linux provides the **Userspace I/O** ( :xref-linux-uio:`uio<>` ) and 
**Virtual Function I/O** :xref-linux-vfio:`vfio<>` frameworks to write user
space I/ O drivers. Both interfaces work by binding a given device to an
in-kernel stub-driver. The stub-driver in turn exposes device-memory and
device-interrupts to user space. Thus enabling the implementation of device
drivers entirely in user space.

Although Linux provides a capable NVMe Driver with flexible IOCTLs, then a user
space NVMe driver serves those who seek the lowest possible per-command
processing overhead or wants full control over NVMe command construction,
including command-payloads.

Fortunately, you do not need to go and write an user space NVMe driver
since a highly efficient, mature and well-maintained driver already exists.
Namely, the NVMe driver provided by the **Storage Platform Development Kit**
(:xref-spdk:`SPDK<>`).

Another great fortune is that **xNVMe** bundles the SPDK NVMe Driver with the
**xNVMe** library. So, if you have built and installed **xNVMe** then the
**SPDK** NVMe Driver is readily available to **xNVMe**.

The following subsections goes through a configuration checklist, then shows
how to bind and unbind drivers, and lastly how to utilize non-devfs device
identifiers by enumerating the system and inspecting a device.

.. _sec-backends-spdk-config-userspace-config:

Config
~~~~~~

What remains is checking your system configuration, enabling IOMMU for use by
the ``vfio-pci`` driver, and possibly falling back to the ``uio_pci_generic``
driver in case ``vfio-pci`` is not working out.  ``vfio`` is preferred as
hardware support for IOMMU allows for isolation between devices.

1) Verify that your CPU supports virtualization / VT-d and that it is enabled
   in your board BIOS.

2) Enable your kernel for an intel CPU then provide the kernel option
   ``intel_iommu=on``.  If you have a non-Intel CPU then consult documentation
   on enabling VT-d / IOMMU for your CPU.

3) Increase limits, open ``/etc/security/limits.conf`` and add:

::

  *    soft memlock unlimited
  *    hard memlock unlimited
  root soft memlock unlimited
  root hard memlock unlimited

Once you have gone through these steps, and rebooted, then this command:

.. literalinclude:: 200_dmesg.cmd
   :language: bash

Should output:

.. literalinclude:: 200_dmesg.out
   :language: bash

And this command:

.. literalinclude:: 300_find.cmd
   :language: bash

Should have output similar to:

.. literalinclude:: 300_find.out
   :language: bash

Unbinding and binding
~~~~~~~~~~~~~~~~~~~~~

With the system configured then you can use the ``xnvme-driver`` script to bind
and unbind devices. The ``xnvme-driver`` script is a merge of the **SPDK**
``setup.sh`` script and its dependencies.

By running the command below **8GB** of hugepages will be configured, the
Kernel NVMe driver unbound, and ``vfio-pci`` bound to the device:

.. literalinclude:: 010_xnvme_driver.cmd
   :language: bash

The command above should produce output similar to:

.. literalinclude:: 010_xnvme_driver.out
   :language: bash

To unbind from ``vfio-pci`` and back to the Kernel NVMe driver, then run:

.. literalinclude:: 500_xnvme_driver_reset.cmd
   :language: bash

Should output similar to:

.. literalinclude:: 500_xnvme_driver_reset.out
   :language: bash
