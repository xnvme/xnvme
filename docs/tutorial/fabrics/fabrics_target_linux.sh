#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_target_modules.sh

echo "# Ensure that the NVMe devices are associated with the Linux Kernel NVMe driver"
xnvme-driver reset

echo "# Mounting configfs"
/bin/mount -t configfs none /sys/kernel/config/

echo "# Create an NVMe Target Subsystem"
mkdir -p "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}"

echo "# Set subsystem access to 'attr_allow_any_host'"
echo 1 > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}/attr_allow_any_host"

echo "# Create a NVMe Namespace within the Target Subsystem"
mkdir -p "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}/namespaces/1"

echo "# Export (${EXPORT_DEV_PATH}) -- add device to kernel subsystem"
echo -n "${EXPORT_DEV_PATH}" > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}/namespaces/1/device_path"

# Enable the NVMe Target Namespace
echo 1 > "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}/namespaces/1/enable"

echo "## Setup NVMe-oF connection-listener"
echo "# Create a 'port'"
mkdir /sys/kernel/config/nvmet/ports/1

echo "# Set the transport addr (traddr)"
echo "${NVMET_TRADDR}" > "/sys/kernel/config/nvmet/ports/1/addr_traddr"

echo "# Set the transport type"
echo "${NVMET_TRTYPE}" > "/sys/kernel/config/nvmet/ports/1/addr_trtype"

echo "# Set the transport service-id"
echo "${NVMET_TRSVCID}" > "/sys/kernel/config/nvmet/ports/1/addr_trsvcid"

echo "# Set the address-family"
echo "${NVMET_ADRFAM}" > "/sys/kernel/config/nvmet/ports/1/addr_adrfam"

echo "# Link with the subsystem with the port, thereby enabling it"
ln -s "/sys/kernel/config/nvmet/subsystems/${NVMET_SUBSYS_NQN}" \
  "/sys/kernel/config/nvmet/ports/1/subsystems/${NVMET_SUBSYS_NQN}"
