#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_initiator_modules.sh

echo "## Using nvme-cli and xNVMe"

echo "# Show devices using 'xnvme enum' before to connecting/mounting"
xnvme enum

echo "# Discover fabrics target"
nvme discover \
  --transport="${NVMET_TRTYPE}" \
  --traddr="${NVMET_TRADDR}" \
  --trsvcid="${NVMET_TRSVCID}"

echo "# Connect, mount the namespace as block device"
nvme connect \
  --transport="${NVMET_TRTYPE}" \
  --traddr="${NVMET_TRADDR}" \
  --trsvcid="${NVMET_TRSVCID}" \
  --nqn="${NVMET_SUBSYS_NQN}"

# Time to settle...
sleep 2

echo "# Show devices after connecting/mounting"
xnvme enum

for dev_path in ${NVMEI_DEVPATHS}; do
  echo "# Inspect dev_path: '${dev_path}' using xNVMe"
  xnvme info "${dev_path}"

  echo "# Run fio"
  "${XNVME_REPOS}/subprojects/fio/fio" \
    /usr/local/share/xnvme/xnvme-compare.fio \
    --section=default \
    --ioengine="io_uring" \
    --filename="${dev_path}"
done

echo "# Disconnect, unmount the block-device"
nvme disconnect --nqn="${NVMET_SUBSYS_NQN}"
