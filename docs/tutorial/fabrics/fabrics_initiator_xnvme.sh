#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_initiator_modules.sh

echo "# Prepare the SPDK/DPDK environment"
xnvme-driver

echo "# Enumerate the transport"
xnvme enum --uri "${NVMET_TRADDR}:${NVMET_TRSVCID}"

for nsid in $(seq "$(echo "${NVMET_PCIE_IDS}" | wc -w)"); do
  echo "# Inspect nsid: '${nsid}' using xNVMe"
  xnvme info "${NVMET_TRADDR}:${NVMET_TRSVCID}" --dev-nsid="${nsid}"

  echo "# Run fio"
  "${XNVME_REPOS}/subprojects/fio/fio" \
    /usr/local/share/xnvme/xnvme-compare.fio \
    --section=default \
    --ioengine="external:$(pkg-config xnvme --variable=libdir)/libxnvme-fio-engine.so" \
    --filename="${NVMET_TRADDR}\\:${NVMET_TRSVCID}" \
    --xnvme_dev_nsid="${nsid}"
done
