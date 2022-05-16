#!/usr/bin/env bash
#
# Verify that CLI `kvs list` runs without error
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
XNVME_RT_ARGS="${XNVME_RT_ARGS} --nsid ${XNVME_DEV_NSID}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --dev-nsid ${XNVME_DEV_NSID}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --be ${XNVME_BE}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --admin ${XNVME_ADMIN}"

: "${KV_KEY_FROM:="hello"}"
: "${KV_VAL_FROM:="world"}"

: "${KV_KEY_REST:="marco"}"
: "${KV_VAL_REST:="polo"}"


if ! cij.cmd "kvs delete ${XNVME_URI} ${XNVME_RT_ARGS} --key ${KV_KEY_FROM}"; then
  cij.info "kvs delete failed; totally expected, just making sure key ${KV_KEY_FROM} didn't exist"
fi

if ! cij.cmd "kvs delete ${XNVME_URI} ${XNVME_RT_ARGS} --key ${KV_KEY_REST}"; then
  cij.info "kvs delete failed; totally expected, just making sure key ${KV_KEY_REST} didn't exist"
fi

if ! cij.cmd "kvs store ${XNVME_URI} ${XNVME_RT_ARGS} --key ${KV_KEY_FROM} --value ${KV_VAL_FROM}"; then
  test.fail
fi

if ! cij.cmd "kvs store ${XNVME_URI} ${XNVME_RT_ARGS} --key ${KV_KEY_REST} --value ${KV_VAL_REST}"; then
  test.fail
fi

if ! cij.cmd "kvs list ${XNVME_URI} ${XNVME_RT_ARGS} --key ${KV_KEY_FROM} | hexdump"; then
  test.fail
fi

test.pass
