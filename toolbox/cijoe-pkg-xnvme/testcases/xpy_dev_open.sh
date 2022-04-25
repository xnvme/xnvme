#!/bin/bash
#
# Verify that `xpy_dev_open` runs without error
#
# When this passes the xNVMe Python bindings loadable and functional.
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

# Instrumentation of the xNVMe runtime
XNVME_RT_ARGS=""
XNVME_RT_ARGS="${XNVME_RT_ARGS} --dev-nsid ${XNVME_DEV_NSID}"

if ! cij.cmd "xpy_dev_open --uri ${XNVME_URI} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
