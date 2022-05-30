#!/usr/bin/env bash
#
# Variables defining the fabrics setup, adjust these to match your setup
#
export NVMET_SUBNQN="nqn.2022-06.io.xnvme:ctrlnode"

export NVMET_TRADDR="1.2.3.4"
export NVMET_TRTYPE="tcp"
export NVMET_TRSVCID="4420"
export NVMET_ADRFAM="ipv4"

# Device names and PCIe identifiers of NVMe devices on the NVMe-over-Fabrics target
export NVMET_DEVPATHS="/dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1"
export NVMET_PCIE_IDS="0000:41:00.0 0000:42:00.0 0000:43:00.0 0000:44:00.0"

export NVMEI_DEVPATHS="/dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme3n1"

# Absolute path to the xNVMe repository
export XNVME_REPOS="/root/git/xnvme"
