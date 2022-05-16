#!/usr/bin/env bash
#
# Verify that the CLI `xnvme idfy` runs without error
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${XNVME_URI:?Must be set and non-empty}"

: "${XNVME_DEV_NSID:?Must be set and non-empty}"
: "${XNVME_BE:?Must be set and non-empty}"
: "${XNVME_ADMIN:?Must be set and non-empty}"

# Instrumentation of the xNVMe runtime
XNVME_RT_ARGS=""
XNVME_RT_ARGS="${XNVME_RT_ARGS} --dev-nsid ${XNVME_DEV_NSID}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --be ${XNVME_BE}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --admin ${XNVME_ADMIN}"

: "${CMD_CNS:=0x0}"
: "${CMD_CNTID:=0x0}"
: "${CMD_NSID:=${XNVME_DEV_NSID}}"
: "${CMD_SETID:=0x0}"
: "${CMD_UUID:=0x0}"

if ! cij.cmd "xnvme idfy ${XNVME_URI} --cns ${CMD_CNS} --cntid ${CMD_CNTID} --nsid ${CMD_NSID} --setid ${CMD_SETID} --uuid ${CMD_UUID} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
