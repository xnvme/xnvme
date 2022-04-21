#!/usr/bin/env bash

# CIJOE: SSH_* environment variables; comment out to match your test-target machine
#: "${SSH_HOST=localhost}"; export SSH_HOST
#: "${SSH_PORT:=22}"; export SSH_PORT
#: "${SSH_USER:=root}"; export SSH_USER
#: "${SSH_NO_CHECKS:=1}"; export SSH_NO_CHECKS
#

# NVMe-over-Fabrics environment variables
#
# See the XNVME_URI-construction below and the "spdk_nvmef" hook for details on how these variables
# are used
#
#NVMEF_TRANSPORT="TCP"; export NVMEF_TRANSPORT
#NVMEF_TARGET_ADDR="IP-address-of-NVMEF-target"; export NVMEF_TARGET_ADDR
#NVMEF_TARGET_PORT="4420"; export NVMEF_TARGET_PORT
#NVMEF_TARGET_XNVME_REPOS_PATH="/somewhere/on/the/fabrics/host"; export NVMEF_TARGET_XNVME_REPOS_PATH
#NVMEF_TARGET_SSH_HOST="${NVMEF_TARGET_ADDR}"; export NVMEF_TARGET_SSH_HOST
#NVMEF_TARGET_SSH_PORT="${SSH_PORT}"; export NVMEF_TARGET_SSH_PORT
#

#
# PCIe and NVMe info; setup identifying the PCI device to use, the NVMe-
#

# Select other values based on 'NVME_NSTYPE' defined in testplan
PCI_DEV_NAME="0000:03:00.0"; export PCI_DEV_NAME
NVME_CNTID="0"; export NVME_CNTID
if [[ -v NVME_NSTYPE && "${NVME_NSTYPE}" == "lblk" ]]; then
  NVME_NSID="1"; export NVME_NSID
elif [[ -v NVME_NSTYPE && "${NVME_NSTYPE}" == "zoned" ]]; then
  NVME_NSID="2"; export NVME_NSID
elif [[ -v NVME_NSTYPE && "${NVME_NSTYPE}" == "kvs" ]]; then
  NVME_NSID="3"; export NVME_NSID
else
  NVME_NSID="1"; export NVME_NSID
fi

#
# xNVMe: define XNVME_URI based on XNVME_BE and DEV_TYPE
#
if [[ -v XNVME_BE && "$XNVME_BE" == "spdk" ]]; then
  if [[ -v NVMEF_TARGET_ADDR && -v NVMEF_TARGET_PORT ]]; then
    XNVME_URI="${NVMEF_TARGET_ADDR}:${NVMEF_TARGET_PORT}"; export XNVME_URI
  else
    XNVME_URI="${PCI_DEV_NAME}"; export XNVME_URI
  fi
elif [[ -v XNVME_BE && "${XNVME_BE}" == "linux" ]]; then
  if [[ -v DEV_TYPE && "${DEV_TYPE}" == "nullblk" ]]; then
    case $NVME_NSTYPE in
    lblk)
      XNVME_URI="/dev/nullb0"; export XNVME_URI
      ;;
    zoned)
      XNVME_URI="/dev/nullb1"; export XNVME_URI
      ;;
    esac
  elif [[ -v DEV_TYPE && "${DEV_TYPE}" == "char" ]]; then
    XNVME_URI="/dev/ng${NVME_CNTID}n${NVME_NSID}"; export XNVME_URI
  else
    XNVME_URI="/dev/nvme${NVME_CNTID}n${NVME_NSID}"; export XNVME_URI
  fi
elif [[ -v XNVME_BE && "$XNVME_BE" == "fbsd" ]]; then
  XNVME_URI="/dev/nvme${NVME_CNTID}ns${NVME_NSID}"; export XNVME_URI
elif [[ -v XNVME_BE && "$XNVME_BE" == "macos" ]]; then
  XNVME_URI="/dev/disk4"; export XNVME_URI
fi
if [[ -v XNVME_BE ]]; then
  XNVME_DEV_NSID="${NVME_NSID}"; export XNVME_DEV_NSID
fi

# xNVMe: where are libraries and share stored on the target system? This is
# needed to find the xNVMe fio io-engine, fio scripts etc.
#
# Consult: 'pkg-config --variable="libdir" xnvme' on the target system
: "${XNVME_LIB_ROOT:=/usr/local/lib/x86_64-linux-gnu}"; export XNVME_LIB_ROOT
# Consult: 'pkg-config --variable="datadir" xnvme' on the target system
: "${XNVME_SHARE_ROOT:=/usr/local/share/xnvme}"; export XNVME_SHARE_ROOT

# These are for running fio
if [[ -v NVME_NSTYPE ]]; then
  : "${SPDK_FIOE_ROOT:=/opt/aux}"; export SPDK_FIOE_ROOT
fi

# This is for the SPDK setup.sh script and xNVMe xnvme-driver script
# It can be overwritten in the testplan
HUGEMEM="${HUGEMEM:=2048}"; export HUGEMEM

# The external fio io-engines usually depend on the fio-version on which they
# built against. So, we set the FIO_BIN to point it to the version that comes
# with the xNVMe dockerize reference environment
: "${FIO_BIN:=/opt/aux/fio}"; export FIO_BIN

#
# These are for the nullblock hook, specifically when loading it
#
: "${NULLBLK_NR_DEVICES=0}"; export NULLBLK_NR_DEVICES
