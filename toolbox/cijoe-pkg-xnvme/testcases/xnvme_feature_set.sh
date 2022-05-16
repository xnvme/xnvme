#!/usr/bin/env bash
#
# Verify that the TEST `xnvme feature-set` runs without error
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

: "${CMD_FID:=0x4}"
: "${CMD_FEAT:=0x1}"
: "${CMD_SAVE:=--save}"

if ! cij.cmd "xnvme feature-set ${XNVME_URI} --fid ${CMD_FID} --feat ${CMD_FEAT} ${CMD_SAVE} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
