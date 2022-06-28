#!/usr/bin/env bash
#
# Set up and initialize the localhost target
# REQUIRES:
# - NVMET_IP
#   The ip of the target, in this case it should be: 127.0.0.1
# - NVMET_PORT
#   The port to export the devices, typically this is: 4420
# - NVMET_SUBNQN_PREFIX
#   The prefix of the nqn of the subsystem exported i.e.: "nqn.2022-06.io.xnvme:cnode"
#   NOTE: a regular nqn includes a number at the end i.e.: "nqn.2022-06.io.xnvme:cnode1", this is added dynamically in this hook
# - NVMET_PCIE_IDS
#   The device ids separated by a space i.e.: "0000:02:00.0 0000:04:00.0"
# - XNVME_REPO
#   This is needed in to locate the SPDK "nvmf" binary and "rpc.py" script inside the xNVMe repos,
#   such that the cijoe-hook can SSH into the machine, start the listener and export devices
#
CIJ_TEST_NAME="$(basename "${BASH_SOURCE[0]}")"
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

hook.spdk_nvmf_localhost_enter() {
    if [[ ! -v NVMET_IP ]]; then
        cij.error "spdk_nvmf_localhost_enter: Missing: NVMET_IP"
        return 1
    fi
    if [[ ! -v NVMET_PORT ]]; then
        cij.error "spdk_nvmf_localhost_enter: Missing: NVMET_PORT"
        return 1
    fi
    if [[ ! -v NVMET_PCIE_IDS ]]; then
        cij.error "spdk_nvmf_localhost_enter: Missing: NVMET_PCIE_IDS"
        return 1
    fi
    if [[ ! -v XNVME_REPO ]]; then
        cij.error "spdk_nvmf_localhost_enter: Missing: XNVME_REPO"
        return 1
    fi

    local rpc="${XNVME_REPO}/subprojects/spdk/scripts/rpc.py"
    local trtype="tcp"
    local adrfam="ipv4"

    cij.info "hook:spdk_nvmf_localhost_enter: Setting up NVMe-oF localhost target"

    # Load modules
    if ! cij.cmd "modprobe nvme && modprobe nvmet && modprobe nvmet_tcp && modprobe nvme_fabrics && modprobe nvme_tcp"; then
        cij.err "spdk_nvmf_localhost_enter: failed to load modules"
    fi

    # Load xnvme SPDK driver
    if ! cij.cmd "xnvme-driver"; then
        cij.err "spdk_nvmf_localhost_enter: failed SPDK initialization"
        return 1
    fi

    # Build the SPDK NVMe-oF target-app (nvmf_tgt)
    if ! cij.cmd "cd ${XNVME_REPO}/subprojects/spdk/app/nvmf_tgt && make"; then
        cij.err "spdk_nvmf_localhost_enter: failed to build 'nvmf_tgt'"
        return 1
    fi

    # Kill nvmf_tgt if it is already running
    if ! cij.cmd "pkill -f nvmf_tgt"; then
        cij.info "spdk_nvmf_localhost_enter: failed killing 'nvmf_tgt'... that is ok. Probably just means it was not running."
    fi

    # Start 'nvmf_tgt' 
    if ! cij.cmd "cd ${XNVME_REPO}/subprojects/spdk/build/bin && (nohup ./nvmf_tgt > foo.out 2> foo.err < /dev/null &)"; then
        cij.err "spdk_nvmf_localhost_enter: failed to start 'nvmf_tgt'"
        return 1
    fi

    # ... and give it two seconds to settle
    sleep 2

    # Create transport
    if ! cij.cmd "${rpc} nvmf_create_transport -t ${trtype} -u 16384 -m 8 -c 8192"; then
        cij.err "spdk_nvmf_localhost_enter: failed to create transport"
        return 1
    fi  

    # Create subsystems, attach controllers and add listener 
    count=0
    for pcie_id in ${NVMET_PCIE_IDS}; do
        count=$((count + 1))

        if ! cij.cmd "${rpc} nvmf_create_subsystem ${NVMET_SUBNQN_PREFIX}${count} -a -s SPDK0000000000000${count} -d Controller${count}"; then
            cij.err "spdk_nvmf_localhost_enter: failed to create subsystem: ${NVMET_SUBNQN_PREFIX}${count}"
            return 1
        fi

        if ! cij.cmd "${rpc} bdev_nvme_attach_controller -b Nvme${count} -t PCIe -a ${pcie_id}"; then
            cij.err "spdk_nvmf_localhost_enter: failed to attach controller: ${pcie_id}"
            return 1
        fi

        if ! cij.cmd "${rpc} nvmf_subsystem_add_ns ${NVMET_SUBNQN_PREFIX}${count} Nvme${count}n1"; then
            cij.err "spdk_nvmf_localhost_enter: failed to add namespace ${NVMET_SUBNQN_PREFIX}${count}: Nvme${count}n1"
            return 1
        fi

        if ! cij.cmd "${rpc} nvmf_subsystem_add_listener ${NVMET_SUBNQN_PREFIX}${count} -t ${trtype} -a ${NVMET_IP} -s ${NVMET_PORT} -f ${adrfam}"; then
            cij.err "spdk_nvmf_localhost_enter: failed to add listener to ${NVMET_SUBNQN_PREFIX}${count}: ${NVMET_IP}:${NVMET_PORT}"
            return 1
        fi
    done

    return 0
}
hook.spdk_nvmf_localhost_enter
exit $?
