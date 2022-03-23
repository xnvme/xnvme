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

hook.xnvme_exit() {
  if ! xnvme.env; then
    cij.err "xnvme.env - Invalid xNVMe env."
    return 1
  fi

  if [[ ! -v XNVME_BE || -z "${XNVME_BE}" ]]; then
    cij.err "hook.xnvme_exit: XNVME_BE is not set or is the empty string"
    return 1
  fi

  cij.info "hook:xnvme_exit: XNVME_BE(${XNVME_BE})"

  if ! cij.cmd "xnvme-driver reset"; then
    cij.err "hook.xnvme_exit: FAILED: setting NVMe drivers"
    return 1
  fi

  return 0
}

hook.xnvme_exit
exit $?
