#!/usr/bin/env bash
#
# Set up a PCIe Configuration with:
#
# - Single controller
# - Two namespaces
#   - One NVM Namespace
#   - One Zoned Namespace
#
qemu_setup_pcie() {
  local _ctrlr_mdts=7
  local _ctrlr_ident="nvme0"

  local _ns_nvm_nsid=1
  local _ns_nvm_lbads=12
  local _ns_nvm_size="8G"
  local _ns_nvm_ident="${_ctrlr_ident}n${_ns_nvm_nsid}"

  local _ns_zns_nsid=2
  local _ns_zns_lbads=12
  local _ns_zns_size="8G"
  local _ns_zns_ident="${_ctrlr_ident}n${_ns_zns_nsid}"

  local _args=""

  # Create the PCIe configuration args
  _args="${_args} -device pcie-root-port,id=pcie_root_port1,chassis=1,slot=1"
  _args="${_args} -device x3130-upstream,id=pcie_upstream_port1,bus=pcie_root_port1"
  _args="${_args} -device xio3130-downstream,id=pcie_downstream_port1,bus=pcie_upstream_port1,chassis=2,slot=1"

  # Create the NVMe Controller configuration args
  _args="${_args} -device nvme,id=${_ctrlr_ident},serial=deadbeef,bus=pcie_downstream_port1,mdts=${_ctrlr_mdts}"

  # Create the NVM Namespace backing image and QEMU configuration args
  qemu.img_create "${_ns_nvm_ident}" "raw" "${_ns_nvm_size}"
  _args="${_args} $(qemu.args_drive ${_ns_nvm_ident} raw)"

  _args="${_args} -device nvme-ns,id=${_ns_nvm_ident},drive=${_ns_nvm_ident},bus=${_ctrlr_ident},nsid=${_ns_nvm_nsid},lbads=${_ns_nvm_lbads}"

  # Create the Zoned Namepsace backing images and QEMU configuration args
  qemu.img_create "${_ns_zns_ident}" "raw" "${_ns_zns_size}"
  _args="${_args} $(qemu.args_drive ${_ns_zns_ident} raw)"

  qemu.img_create "${_ns_zns_ident}-zoneinfo" "raw" "0"
  _args="${_args} $(qemu.args_drive ${_ns_zns_ident}-zoneinfo raw)"

  _args="${_args}  -device nvme-ns,id=${_ns_zns_ident},drive=${_ns_zns_ident},bus=${_ctrlr_ident},nsid=${_ns_zns_nsid},lbads=${_ns_zns_lbads}"
  _args="${_args},iocs=0x2,zns.zcap=4096,zns.zoneinfo=${_ns_zns_ident}-zoneinfo"

  # ZNS specific args
  _args="${_args},zns.zdes=0"
  _args="${_args},zns.zoc=0"
  _args="${_args},zns.mar=383"
  _args="${_args},zns.mor=383"

  : "${QEMU_SETUP_PCIE=${_args}}"; export QEMU_SETUP_PCIE
}
