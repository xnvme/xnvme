#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_target_modules.sh

echo "# Ensure that the NVMe devices are associated with the Linux Kernel NVMe driver"
xnvme-driver reset

echo "# Mounting configfs"
/bin/mount -t configfs none /sys/kernel/config/ || echo "Possibly already mounted"

port=1

echo "## Setup NVMe-oF connection-listener"
echo "# Create a 'port'"
mkdir "/sys/kernel/config/nvmet/ports/${port}"

echo "# Set the transport addr (traddr)"
echo "${NVMET_TRADDR}" > "/sys/kernel/config/nvmet/ports/${port}/addr_traddr"

echo "# Set the transport type"
echo "${NVMET_TRTYPE}" > "/sys/kernel/config/nvmet/ports/${port}/addr_trtype"

echo "# Set the transport service-id"
echo "${NVMET_TRSVCID}" > "/sys/kernel/config/nvmet/ports/${port}/addr_trsvcid"

echo "# Set the address-family"
echo "${NVMET_ADRFAM}" > "/sys/kernel/config/nvmet/ports/${port}/addr_adrfam"

count=0
for dev_path in ${NVMET_DEVPATHS}; do
  count=$((count + 1))
  subnqn="${NVMET_SUBNQN}${count}"
  nsid=1

  echo "# Creating an NVMe Target Subsystem for each device"
  mkdir -p "/sys/kernel/config/nvmet/subsystems/${subnqn}"

  echo "# Set subsystem access to 'attr_allow_any_host'"
  echo 1 > "/sys/kernel/config/nvmet/subsystems/${subnqn}/attr_allow_any_host"

  echo "# Create a NVMe Namespace within the Target Subsystem"
  mkdir -p "/sys/kernel/config/nvmet/subsystems/${subnqn}/namespaces/${nsid}"

  echo "# Export (${dev_path}) -- add device to kernel subsystem"
  echo -n "${dev_path}" > "/sys/kernel/config/nvmet/subsystems/${subnqn}/namespaces/${nsid}/device_path"

  # Enable the NVMe Target Namespace
  echo 1 > "/sys/kernel/config/nvmet/subsystems/${subnqn}/namespaces/${nsid}/enable"

  echo "# Link with the subsystem with the port, thereby enabling it"
  ln -s "/sys/kernel/config/nvmet/subsystems/${subnqn}" \
   "/sys/kernel/config/nvmet/ports/${port}/subsystems/${subnqn}"
done
