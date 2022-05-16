#!/usr/bin/env bash
#
# Verify that CLI `xnvme log-erri` runs without error
#
# Primitive check to verify functionality of `xnvme log-erri`.
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

: "${CMD_NSID:=0x0}"

if ! cij.cmd "xnvme log-erri ${XNVME_URI} --nsid ${CMD_NSID} --data-output /tmp/xnvme_log-erri.bin ${XNVME_RT_ARGS}"; then
  test.fail
fi

# Grab the log-output
ssh.pull "/tmp/xnvme_log-erri.bin" "${CIJ_TEST_AUX_ROOT}/"

test.pass
