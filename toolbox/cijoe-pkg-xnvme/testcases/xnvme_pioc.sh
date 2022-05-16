#!/usr/bin/env bash
#
# Verify that CLI `xnvme pioc` runs without error
#
# NOTE: we are using the same namespace-identifier as used for the device.
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

: "${CMD_NSID:=${XNVME_DEV_NSID}}"
: "${CMD_OPCODE:=0x02}"     # read-command on I/O queue

if ! cij.cmd "xnvme info ${XNVME_URI} ${XNVME_RT_ARGS} > /tmp/device-info.yml"; then
  test.fail
fi
if ! ssh.pull "/tmp/device-info.yml" "/tmp/device-info.yml"; then
  test.fail
fi

CMD_DATA_NBYTES=$(cat < /tmp/device-info.yml | awk '/lba_nbytes:/ {print $2}')

CMD_FPATH=$(mktemp -u --tmpdir=/tmp -t read_XXXXXX.nvmec)

if ! cij.cmd "nvmec create --opcode ${CMD_OPCODE} --nsid ${CMD_NSID} --cmd-output ${CMD_FPATH}"; then
  test.fail
fi

if ! cij.cmd "nvmec show --cmd-input ${CMD_FPATH}"; then
  test.fail
fi

if ! cij.cmd "xnvme pioc ${XNVME_URI} --cmd-input ${CMD_FPATH} --data-nbytes ${CMD_DATA_NBYTES} ${XNVME_RT_ARGS}"; then
  test.fail
fi

test.pass
