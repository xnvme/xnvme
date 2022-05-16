#!/usr/bin/env bash
#
# Verify that CLI `xnvme log` runs without error
#
# Primitive check to verify functionality of `xnvme log`, it requests the
# error-log as this is a log which should be easily retrieved as it is mandatory
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

: "${LID:=0x1}"
: "${LSP:=0x0}"
: "${LPO_NBYTES:=0}"
: "${RAE:=0}"
: "${NBYTES:=4096}"

if ! cij.cmd "xnvme log ${XNVME_URI} --lid ${LID} --lsp ${LSP} --lpo-nbytes ${LPO_NBYTES} --rae ${RAE} --data-nbytes ${NBYTES} --data-output /tmp/xnvme-log.bin ${XNVME_RT_ARGS}"; then
  test.fail
fi

# Grab the log-output
ssh.pull "/tmp/xnvme-log.bin" "${CIJ_TEST_AUX_ROOT}/"

test.pass
