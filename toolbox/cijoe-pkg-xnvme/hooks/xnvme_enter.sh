#!/usr/bin/env bash
#
# Verifies that environment variables required by the xNVMe testcases are
# defined and detaches / re-attaches NVMe devices from kernel to user space
#
# on-enter: de-attach from kernel NVMe driver
# on-exit: re-attach to kernel NVMe driver
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

hook.xnvme_enter() {
  if ! xnvme.env; then
    cij.err "xnvme.env - Invalid xNVMe env."
    return 1
  fi

  if [[ ! -v XNVME_BE || -z "${XNVME_BE}" ]]; then
    cij.err "hook.xnvme_enter: XNVME_BE is not set or is the empty string"
    return 1
  fi

  cij.info "hook:xnvme_enter: XNVME_BE(${XNVME_BE})"

  XNVME_DRIVER_CMD="xnvme-driver"

  case $XNVME_BE in
  spdk|SPDK)
    if [[ -v HUGEMEM && -n "${HUGEMEM}" ]]; then
      XNVME_DRIVER_CMD="HUGEMEM=${HUGEMEM} ${XNVME_DRIVER_CMD}"
    fi
    if [[ -v NRHUGE && -n "${NRHUGE}" ]]; then
      XNVME_DRIVER_CMD="NRHUGE=${NRHUGE} ${XNVME_DRIVER_CMD}"
    fi
    if [[ -v HUGENODE && -n "${HUGENODE}" ]]; then
      XNVME_DRIVER_CMD="HUGENODE=${HUGENODE} ${XNVME_DRIVER_CMD}"
    fi
    ;;

  *)
    XNVME_DRIVER_CMD="${XNVME_DRIVER_CMD} reset"
  esac

  if ! cij.cmd "${XNVME_DRIVER_CMD}"; then
    cij.err "hook.xnvme_enter: FAILED: detaching NVMe driver"
    return 1
  fi

  return 0
}

hook.xnvme_enter
exit $?
