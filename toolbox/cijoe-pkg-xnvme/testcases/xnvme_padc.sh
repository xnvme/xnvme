#!/usr/bin/env bash
#
# Verify that CLI `xnvme padc` runs without error
#
# Send and identify-controller via cli-passthru-interface 'xnvme padc'
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

# Setup args for instrumentation of the xNVMe runtime
XNVME_RT_ARGS=""
XNVME_RT_ARGS="${XNVME_RT_ARGS} --dev-nsid ${XNVME_DEV_NSID}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --be ${XNVME_BE}"
XNVME_RT_ARGS="${XNVME_RT_ARGS} --admin ${XNVME_ADMIN}"

: "${CMD_OPCODE:=0x02}"     # Identify
: "${CMD_CNS:=0x1}"         # Identify-controller
: "${CMD_DATA_NBYTES:=4096}"

CMD_FPATH=$(mktemp -u --tmpdir=/tmp -t glp_XXXXXX.nvmec)

if ! cij.cmd "nvmec create --opcode ${CMD_OPCODE} --cdw10 ${CMD_CNS} --cmd-output ${CMD_FPATH}"; then
  test.fail
fi

if ! cij.cmd "nvmec show --cmd-input ${CMD_FPATH}"; then
  test.fail
fi

cij.info "Passing command through"

if ! cij.cmd "xnvme padc ${XNVME_URI} --cmd-input ${CMD_FPATH} --data-nbytes ${CMD_DATA_NBYTES} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
