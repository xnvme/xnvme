#!/usr/bin/env bash
#
# Shutdown the target controller
#
CIJ_TEST_NAME="$(basename "${BASH_SOURCE[0]}")"
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

hook.spdk_nvmf_localhost_exit() {
    cij.info "hook:spdk_nvmf_localhost_exit: Kill target controller"
    if ! cij.cmd "pkill -f nvmf_tgt"; then
        cij.err "spdk_nvmf_localhost_exit: Failed killing nvmf_tgt"
        return 1;
    fi
    return 0
}

hook.spdk_nvmf_localhost_exit
exit $?
