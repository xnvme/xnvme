#!/bin/bash
#
# Verify that `xnvme_hello` runs without error
#
# Primitive check to verify that the example code is valid
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
: "${XNVME_SYNC:?Must be set and non-empty}"
: "${XNVME_ASYNC:?Must be set and non-empty}"
: "${XNVME_ADMIN:?Must be set and non-empty}"
: "${XNVME_DIRECT:?Must be set and non-empty}"
: "${XNVME_VEC_CNT:?Must be set and non-empty}"

# Instrumentation of the xNVMe runtime
XNVME_RT_ARGS=""
XNVME_RT_ARGS="${XNVME_RT_ARGS} --dev-nsid ${XNVME_DEV_NSID}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --be ${XNVME_BE}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --admin ${XNVME_ADMIN}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --sync ${XNVME_SYNC}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --async ${XNVME_ASYNC}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --direct ${XNVME_DIRECT}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --vec-cnt ${XNVME_VEC_CNT}"

if ! cij.cmd "xnvme_tests_ioworker verify ${XNVME_URI} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
