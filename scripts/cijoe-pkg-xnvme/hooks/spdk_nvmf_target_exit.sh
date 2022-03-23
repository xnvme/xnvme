#!/usr/bin/env bash
#
# Shutdown the target controller
#
CIJ_TEST_NAME="$(basename "${BASH_SOURCE[0]}")"
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

hook.spdk_nvmf_target_exit() {
  if [[ ! -v NVMEF_TARGET_ADDR ]]; then
    cij.err "spdk_nvmf_target_exit: Missing: NVMEF_TARGET_ADDR"
    return 1
  fi

  SSH_HOST="${NVMEF_TARGET_ADDR}"; export SSH_HOST

  cij.info "hook:spdk_nvmf_target_exit: Kill target controller"
  if ! cij.cmd "pkill -f \"SCREEN -S fabrics\""; then
    cij.err "spdk_nvmf_target_exit: Failed killing screen"
    return 1;
  fi

  return 0
}

hook.spdk_nvmf_target_exit
exit $?
