.. _sec-tools-xnvme:

xnvme
#####

``xnvme`` is the entry point to the **xNVMe** command line. Run it without
arguments for the commands to start with and the groups the rest are in:

.. literalinclude:: xnvme_overview.cmd
   :language: bash

.. literalinclude:: xnvme_overview.out
   :language: bash

Run a group to list its commands, and append ``--help`` to any command for its
arguments. Commands such as ``perf``, ``homi`` and ``qublk``
are separate programs, which ``xnvme`` looks up on its exec path,
the install directory unless ``XNVME_EXEC_PATH`` names others;
``xnvme --exec-path`` prints it.

``xnvme env`` says, as YAML, whether the host is ready for the userspace
drivers: the IOMMU groups, whether each kernel module is available, built in
and loaded, the hugepages of each size and NUMA node, where hugetlbfs is
mounted, memlock, the device nodes, the NVMe controllers by driver, and the
queues of the kernel ``nvme`` driver, each with a ``fix`` where something is
missing. For the kernel driver that is ``poll_queues``: polled I/O through
``io_uring`` needs it above zero, which takes a module option and a reload of
``nvme``, and a reload drops every NVMe device on the host, so ``xnvme`` says
how rather than doing it. It then prints what ``xnvme library-info`` prints:
the version, the build configuration and the backends. ``xnvme env system``
and ``xnvme env library`` print one of the two. It changes nothing.

``xnvme list`` shows the NVMe controller functions on the PCI bus, found with
``xnvme_scan()``, as YAML: the driver each is on and its IOMMU group, read from
sysfs without opening anything. ``xnvme driver --be <backend> <bdf>`` moves a
controller to the driver the backend needs, and prints the outcome as YAML,
each controller with the driver it is on, whether that changed, and why not.
A backend driving the controller itself, ``spdk``, ``libvfn`` or ``upcie``,
needs a userspace driver: on Linux ``vfio-pci``, or ``uio_pci_generic``
without an IOMMU, and ``vfio-pci`` alone for ``libvfn``; on FreeBSD,
``nic_uio``. Every other backend needs the kernel ``nvme`` driver, so
``--be io_uring`` returns a controller to it. Only the named controller is
rebound, and ``--all`` takes every one not in use. Bare ``xnvme driver``
prints ``xnvme list`` and then ``xnvme env``. One whose
namespaces are mounted, held by device-mapper, md or LVM, used as swap or open
in a process is refused, as is one sharing its IOMMU group with a device on
another driver; one whose namespace carries a filesystem or partition table is
refused unless ``--force``. The binding is done by the platform, through
``xnvme_driver_for_be()``, ``xnvme_driver_attach()`` and
``xnvme_driver_detach()`` in libxnvme, and is not
supported on macOS and Windows. Nothing is recorded on the side: what is
shown is what sysfs says, and whether a controller is in use is checked when
it is to move, which ``--dry-run`` does without moving it.

The installed test programs are listed and run the same way:

.. literalinclude:: xnvme_test.cmd
   :language: bash

.. literalinclude:: xnvme_test.out
   :language: bash

Every built-in command at once:

.. literalinclude:: xnvme_usage.out
   :language: bash

Subcommands
===========

.. toctree::

   enum/index
   feature-get/index
   feature-set/index
   format/index
   idfy-ctrlr/index
   idfy-ns/index
   idfy/index
   info/index
   log-erri/index
   log-health/index
   log/index
   padc/index
   pioc/index
   sanitize/index
