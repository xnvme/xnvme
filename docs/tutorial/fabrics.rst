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

Exporting Targets via kernel modules
------------------------------------

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

  # Creates a port
  mkdir /sys/kernel/config/nvmet/ports/1

  # Set the address (local interface that will be listening)
  echo 172.20.0.100 > "/sys/kernel/config/nvmet/ports/1/addr_traddr"

  # Set the transport type
  echo tcp > "/sys/kernel/config/nvmet/ports/1/addr_trtype"

  # Set the port number
  echo 4420 > "/sys/kernel/config/nvmet/ports/1/addr_trsvcid"

  # Set the address family
  echo ipv4 > "/sys/kernel/config/nvmet/ports/1/addr_adrfam"

  # Create a link and enables the port
  ln -s /sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NAME} /sys/kernel/config/nvmet/ports/1/subsystems/${NVMET_SUBSYS_NAME}


Exporting Targets via SPDK
--------------------------

::
  # Clone xnvme
  git clone https://github.com/OpenMPDK/xNVMe.git

  # Start submodules
  cd xnvme
  git submodule update --init --recursive

  # Build ans install xnvme
  make
  make install-deb

  # Load SPDK drivers
  xnvme-driver

  # Start SPDK Target App (with high priorities)
  cd third-party/spdk/repos/build/bin
  ./nvmf_tgt

  # Create a transport e.g. TCP
  scripts/rpc.py nvmf_create_transport -t TCP -u 16384 -p 8 -c 8192

  # Attach local PCIe controllers (replace the address accordingly)
  scripts/rpc.py bdev_nvme_attach_controller -b Nvme0 -t PCIe -a 0000:03:00.0

  # Create a controller to be exposed over fabrics
  scripts/rpc.py nvmf_create_subsystem nqn.2020-05.io.spdk:cnode1 -a -s SPDK00000000000001 -d SPDK_Controller1

  # Add the local PCIe namespace to the fabrics controller
  scripts/rpc.py nvmf_subsystem_add_ns nqn.2020-05.io.spdk:cnode1 Nvme0n1

  # Listen for connections (replace the transport, address, and port accordingly)
  scripts/rpc.py nvmf_subsystem_add_listener nqn.2020-05.io.spdk:cnode1 -t tcp -a 172.20.0.100 -s 4420


Initiator Setup
===============

Using Kernel support via modules::

  modprobe nvme
  modprobe nvme_fabrics
  modprobe nvme_tcp

If not, then just build a kernel with all the bells-and-whistles.


Using xNVMe SPDK backend::

  # Clone xnvme
  git clone https://github.com/OpenMPDK/xNVMe.git

  # Start submodules
  cd xnvme
  git submodule update --init --recursive

  # Build ans install xnvme
  make
  make install-deb

  # Load SPDK drivers
  xnvme-driver

  # Enumerate fabrics devices (URI example: fab:172.20.0.100:4022?nsid=1)
  zoned enum --uri fab:<addr>:<port>?nsid=<nsid>

  # Test simple I/O in conventional namespaces over fabrics
  xnvme_io_async write fab:<addr>:<port>?nsid=<nsid> --slba 0x0 --elba 0xffff
