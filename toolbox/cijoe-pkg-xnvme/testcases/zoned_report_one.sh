#!/usr/bin/env bash
#
# Verify that CLI `zoned report` runs without error
#
# Basic verification that a report is returned for LIMIT(1) amount, starting
# from somewhere which is not LBA 0x0
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
XNVME_RT_ARGS="${XNVME_RT_ARGS} --sync ${XNVME_ADMIN}"

: "${SLBA:=0x0000000000026400}"
: "${LIMIT:=1}"

if ! cij.cmd "zoned report $XNVME_URI --slba $SLBA --limit $LIMIT ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
