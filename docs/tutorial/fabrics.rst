.. _sec-tutorials-fabrics:

===============================================
 Fabrics non-RDMA TCP transport setup on Linux
===============================================

This guide assumes that you are running Debian Bullseye.

Roles:

* Fabrics Target
* Fabrics Initiator

Target Setup
============

Using Kernel support via modules::

  modprobe nvme
  modprobe nvmet
  modprobe nvmet_tcp

If not, then just build a kernel with all the bells-and-whistles.

With the Kernel support loaded. Then there are a bunch neat tools out there for
configuring your fabrics setup. Here, ``nvme-cli`` is used for the hands-on
approach.

Exporting Targets
-----------------

Define a couple of variables::

  : "${EXPORT_BDNAME:=/dev/nvme0n1}"
  : "${NVMET_SUBSYS_NAME:=jazz}"
  : "${NVMET_SUBSYS_NSID:1}"

We can then export the device at ``EXPORT_BDNAME`` in a NVMe Target Subsystem
named ``NVMET_SUBSYS_NAME`` containing a single NVMe Namespace identified by
``NVMET_SUBSYS_NSID`` without any access control::

  # Configuration is done via kernel user configuration filesystem
  sudo /bin/mount -t configfs none /sys/kernel/config/

  # Create an NVMe Target Subsystem
  mkdir -p "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME}"

  # Allow everybody... this is just for testing...
  echo 1 > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME}/attr_allow_any_host"

  # Create a NVMe Namespace within the Target Subsystem
  mkdir -p "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME}/namespaces/1"

  # Add the device backing the NVMe Namespace
  echo -n "${EXPORT_BDNAME}" > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME}/namespaces/1/device_path"

  # Enable the NVMe Target Namespace
  echo 1 > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME}/namespaces/1/enable"



Initiator Setup
===============

Using Kernel support via modules::

  modprobe nvme
  modprobe nvme_fabrics
  modprobe nvme_tcp

If not, then just build a kernel with all the bells-and-whistles.
