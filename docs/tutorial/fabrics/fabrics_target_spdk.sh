#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_target_modules.sh

echo "### NVMe-oF Fabrics target setup using SPDK"
xnvme-driver

echo "## Build the SPDK NVMe-oF target-app (nvmf_tgt)"
pushd "${XNVME_REPOS}/subprojects/spdk/app/nvmf_tgt"
make
popd

echo "# Start 'nvmf_tgt' and give it two seconds to settle"
pkill -f nvmf_tgt || echo "Failed killing 'nvmf_tgt'; OK, continuing."
pushd "${XNVME_REPOS}/subprojects/spdk/build/bin"
./nvmf_tgt &
popd

sleep 2

if ! pidof nvmf_tgt; then
  echo "## Failed starting 'nvmf_tgt'"
  exit
fi

echo "## Create NVMe-oF transport (${NVMET_TRTYPE})"
"${XNVME_REPOS}/subprojects/spdk/scripts/rpc.py" nvmf_create_transport \
  -t "${NVMET_TRTYPE}" \
  -u 16384 \
  -m 8 \
  -c 8192

echo "## Attach local PCIe controllers (${NVMET_TRTYPE})"
count=0
for pcie_id in ${NVMET_PCIE_IDS}; do
  count=$((count + 1))

  echo "## Create a NVMe-oF subsystem/controller"
  "${XNVME_REPOS}/subprojects/spdk/scripts/rpc.py" nvmf_create_subsystem \
    "${NVMET_SUBNQN}${count}" \
    -a \
    -s "SPDK0000000000000${count}" \
    -d "Controller${count}"

  echo "# Attach pcie_id: '${pcie_id}'"
  # The command below will output e.g. 'Nvme0n1'
  "${XNVME_REPOS}/subprojects/spdk/scripts/rpc.py" bdev_nvme_attach_controller \
    -b "Nvme${count}" \
    -t PCIe \
    -a "${pcie_id}"

  echo "# Export (${pcie_id}) -- add device to SPDK subsystem/controller"
  "${XNVME_REPOS}/subprojects/spdk/scripts/rpc.py" nvmf_subsystem_add_ns \
    "${NVMET_SUBNQN}${count}" \
    "Nvme${count}n1"

  echo "## Setup NVMe-oF connection-listener"
  "${XNVME_REPOS}/subprojects/spdk/scripts/rpc.py" nvmf_subsystem_add_listener \
    "${NVMET_SUBNQN}${count}" \
    -t "${NVMET_TRTYPE}" \
    -a "${NVMET_TRADDR}" \
    -s "${NVMET_TRSVCID}" \
    -f "${NVMET_ADRFAM}"
done
