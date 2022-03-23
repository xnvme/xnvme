#!/usr/bin/env bash
#
# Set up a PCIe Configuration with:
#
# - Single controller
# - Two namespaces
#   - One NVM Namespace 4k lbads
#   - One Zoned Namespace (with ZRWA) 512 lbads
#
qemu_setup_pcie() {
  local _ctrlr_mdts=7
  local _ctrlr_ident="nvme0"

  local _ns_nvm_nsid=1
  local _ns_nvm_lbads=12
  local _ns_nvm_size="8G"
  local _ns_nvm_ident="${_ctrlr_ident}n${_ns_nvm_nsid}"

  local _ns_zns_nsid=2
  local _ns_zns_lbads=9
  local _ns_zns_size="8G"
  local _ns_zns_ident="${_ctrlr_ident}n${_ns_zns_nsid}"

  local _args=""

  # Enable IOMMU
  _args="${_args} -device intel-iommu,pt=on,intremap=on"

  # Create the PCIe configuration args
  _args="${_args} -device pcie-root-port,id=pcie_root_port1,chassis=1,slot=1"
  _args="${_args} -device x3130-upstream,id=pcie_upstream_port1,bus=pcie_root_port1"
  _args="${_args} -device xio3130-downstream,id=pcie_downstream_port1,bus=pcie_upstream_port1,chassis=2,slot=1"

  # Create the NVMe Controller configuration args
  _args="${_args} -device nvme,id=${_ctrlr_ident},serial=deadbeef,bus=pcie_downstream_port1,mdts=${_ctrlr_mdts}"

  # Create the NVM Namespace backing-image and QEMU configuration args
  qemu.img_create "${_ns_nvm_ident}" "raw" "${_ns_nvm_size}"
  _args="${_args} $(qemu.args_drive ${_ns_nvm_ident} raw)"
  _args="${_args} -device nvme-ns,id=${_ns_nvm_ident}"
  _args="${_args},drive=${_ns_nvm_ident}"
  _args="${_args},bus=${_ctrlr_ident}"
  _args="${_args},nsid=${_ns_nvm_nsid}"
  _args="${_args},logical_block_size=$((1 << ${_ns_nvm_lbads}))"
  _args="${_args},physical_block_size=$((1 << ${_ns_nvm_lbads}))"

  # Create the Zoned Namespace backing-image and QEMU configuration args
  qemu.img_create "${_ns_zns_ident}" "raw" "${_ns_zns_size}"
  _args="${_args} $(qemu.args_drive ${_ns_zns_ident} raw)"

  # Namespace setup
  _args="${_args} -device nvme-ns,id=${_ns_zns_ident}"
  _args="${_args},drive=${_ns_zns_ident}"
  _args="${_args},bus=${_ctrlr_ident}"
  _args="${_args},nsid=${_ns_zns_nsid}"
  _args="${_args},logical_block_size=$((1 << ${_ns_zns_lbads}))"
  _args="${_args},physical_block_size=$((1 << ${_ns_zns_lbads}))"

  # zns specific args in lbas
  local _zsze=8192
  local _zcap=4096
  local _foo=256

  # zns specific args
  _args="${_args},zoned=on"
  _args="${_args},zoned.zone_size=$((_zsze << ${_ns_zns_lbads}))"
  _args="${_args},zoned.zone_capacity=$((_zcap << ${_ns_zns_lbads}))"
  _args="${_args},zoned.max_active=${_foo}"
  _args="${_args},zoned.max_open=${_foo}"

  # zns/zrwa specific args
  _args="${_args},zoned.zrwas=$((24 << ${_ns_zns_lbads}))"
  _args="${_args},zoned.zrwafg=$((8 << ${_ns_zns_lbads}))"
  _args="${_args},zoned.numzrwa=${_foo}"

  : "${QEMU_SETUP_PCIE=${_args}}"; export QEMU_SETUP_PCIE
}

