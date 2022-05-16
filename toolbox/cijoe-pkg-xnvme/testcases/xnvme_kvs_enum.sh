#!/usr/bin/env bash
#
# Verify that CLI `kvs enum` runs without error
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
: "${XNVME_SYNC:?Must be set and non-empty}"

# Instrumentation of the xNVMe runtime
XNVME_RT_ARGS=""
XNVME_RT_ARGS="${XNVME_RT_ARGS} --be ${XNVME_BE}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --admin ${XNVME_ADMIN}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --sync ${XNVME_ADMIN}"

if ! cij.cmd "kvs enum ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
