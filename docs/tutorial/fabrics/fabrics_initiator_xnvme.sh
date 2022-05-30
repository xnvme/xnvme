#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_initiator_modules.sh

echo "# Prepare the SPDK/DPDK environment"
xnvme-driver

echo "# Enumerate the transport"
xnvme enum --uri "${NVMET_TRADDR}:${NVMET_TRSVCID}"

for count in $(seq "$(echo "${NVMET_PCIE_IDS}" | wc -w)"); do
  echo "# Inspect controller: '${count}' using xNVMe"
  xnvme info "${NVMET_TRADDR}:${NVMET_TRSVCID}" --dev-nsid=0x1 --subnqn "${NVMET_SUBNQN}${count}"

  echo "# Run fio"
  "${XNVME_REPOS}/subprojects/fio/fio" \
    /usr/local/share/xnvme/xnvme-compare.fio \
    --section=default \
    --ioengine="external:$(pkg-config xnvme --variable=libdir)/libxnvme-fio-engine.so" \
    --filename="${NVMET_TRADDR}\\:${NVMET_TRSVCID}" \
    --xnvme_dev_nsid=1 \
    --xnvme_dev_subnqn="${NVMET_SUBNQN}${count}"
done
