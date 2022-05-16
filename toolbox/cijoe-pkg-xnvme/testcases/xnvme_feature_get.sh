#!/usr/bin/env bash
#
# Verify that the TEST `xnvme feature-get` runs without error
#
# Verifies that is it possible to get feature information via `xnvme gfeat` CLI
# Feature identifier for error-recovery and temperature thresholds are used
#
# NOTE: The select-bit is used in this test, however, it does not error on
# commands using it, since the test does not check whether it is supported
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

for CMD_FID in "0x4" "0x5"; do

  case $CMD_FID in
  "0x4") cij.info "Temperature threshold" ;;
  "0x5") cij.info "Error recovery" ;;
  esac

  cij.info "Getting feature with CMD_FID: ${CMD_FID} without setting select bit"

  if ! cij.cmd "xnvme feature-get $XNVME_URI --fid $CMD_FID ${XNVME_RT_ARGS}"; then
    test.fail
  fi

  cij.info "Getting feature with CMD_FID: ${CMD_FID} and setting select bit"

  for CMD_SEL in "0x0" "0x1" "0x2" "0x3"; do

    case $CMD_SEL in
    "0x0") cij.info "Getting 'current' value for fid: ${CMD_FID}" ;;
    "0x1") cij.info "Getting 'default' value for fid: ${CMD_FID}" ;;
    "0x2") cij.info "Getting 'saved' value for fid: ${CMD_FID}" ;;
    "0x3") cij.info "Getting 'supported' value for fid: ${CMD_FID}" ;;
    esac

    if ! cij.cmd "xnvme feature-get ${XNVME_URI} --fid ${CMD_FID} --sel ${CMD_SEL} ${XNVME_RT_ARGS}"; then
      cij.warn "Device support for select-bit is not checked"
    fi

  done
done

test.pass
