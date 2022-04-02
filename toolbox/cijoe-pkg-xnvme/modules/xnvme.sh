#!/usr/bin/env bash
#
# CIJOE module to verify environment variables for xNVMe testcases
#
# For more information about xNVMe, see: https://xnvme.io/
#
# For documentation of parameters / variables.
#
# xnvme.env    - Checks environment for module parameters
# xnvme.fioe   - Runs fio with the xNVMe fio IO Engine
#
xnvme.env() {
  #: "${XNVME_SHARE_ROOT:=/usr/share/xnvme}"
  export XNVME_SHARE_ROOT

  #: "${XNVME_LIB_ROOT:=/usr/lib}"
  export XNVME_LIB_ROOT

  return 0
}

#
# Uses:
# - xnvme.env
# - XNVME_URI
# - CMD_PREFIX (optional)
# - XNVME_SHARE_ROOT
# - XNVME_LIB_ROOT
#
xnvme.fioe() {
  if [[ -z "$1" || -z "$2" ]]; then
    cij.err "usage: xnvme.fioe('fio-script.fname', 'ioengine_name')"
    cij.err " or"
    cij.err "usage: xnvme.fioe('fio-script.fname', 'ioengine_name', 'section')"
    return 1
  fi

  if ! xnvme.env; then
    cij.err "xnvme:fioe: invalid xNVMe environment"
    return 1
  fi

  : "${CMD_PREFIX:=taskset -c 1 perf stat}"
  : "${FIO_BIN:=fio}"

  local script_fname=$1
  local ioengine_name=$2
  local section=$3
  local output_fpath=$4

  local jobfile="${script_fname}"
  if [[ -v XNVME_SHARE_ROOT && -n "${XNVME_SHARE_ROOT}" ]]; then
    jobfile="${XNVME_SHARE_ROOT}/${jobfile}"
  fi
  local _cmd

  if ! cij.cmd "[[ -f \"${jobfile}\" ]]"; then
    cij.err "xnvme:fioe: '${jobfile}' does not exist!"
    #return 1
  fi

  _cmd="${CMD_PREFIX} ${FIO_BIN} ${jobfile}"
  if [[ -v FIO_BS ]]; then
    _cmd="FIO_BS=${FIO_BS} ${_cmd}"
  fi
  if [[ -v FIO_RW ]]; then
    _cmd="FIO_RW=${FIO_RW} ${_cmd}"
  fi
  if [[ -v FIO_IODEPTH ]]; then
    _cmd="FIO_IODEPTH=${FIO_IODEPTH} ${_cmd}"
  fi
  _cmd="${_cmd} --section=${section}"

  # Add the special-sauce for the external xNVMe fio engine
  if [[ "$ioengine_name" == *"xnvme"* ]]; then
    : "${XNVME_URI:?Must be set and non-empty}"
    : "${XNVME_ASYNC:?Must be set and non-empty}"
    : "${XNVME_SYNC:?Must be set and non-empty}"
    : "${XNVME_ADMIN:?Must be set and non-empty}"

    local _fioe_so
    if [[ -v XNVME_BE && "${XNVME_BE}" == "windows" ]]; then
      _fioe_so="${XNVME_LIB_ROOT}libxnvme-fio-engine.dll"
    else
      _fioe_so="${XNVME_LIB_ROOT}/libxnvme-fio-engine.so"
    fi
    local _fioe_uri=${XNVME_URI//:/\\\\:}

    if ! cij.cmd "[[ -f \"${_fioe_so}\" ]]"; then
      cij.err "xnvme:fioe: '${_fioe_so}' does not exist!"
      #return 1
    fi

    _cmd="${_cmd} --ioengine=external:${_fioe_so}"
    _cmd="${_cmd} --xnvme_async=${XNVME_ASYNC}"
    _cmd="${_cmd} --xnvme_sync=${XNVME_SYNC}"
    _cmd="${_cmd} --xnvme_admin=${XNVME_ADMIN}"
    _cmd="${_cmd} --xnvme_dev_nsid=${XNVME_DEV_NSID}"
    _cmd="${_cmd} --filename=${_fioe_uri}"

  # Add the special-sauce for the external SPDK io-engine spdk_nvme
  elif [[ "$ioengine_name" == "spdk_nvme" ]]; then

    local _traddr=${PCI_DEV_NAME//:/.};

    _cmd="LD_PRELOAD=${SPDK_FIOE_ROOT}/${ioengine_name} ${_cmd}"
    _cmd="${_cmd} --ioengine=spdk"
    _cmd="${_cmd} --filename=\"trtype=PCIe traddr=${_traddr} ns=${NVME_NSID}\""

  # Add the special-sauce for the external SPDK/nvme_bdev
  elif [[ "$ioengine_name" == "spdk_bdev" ]]; then
    # Produce SPDK config-file
    echo "[Nvme]" > /tmp/spdk.bdev.conf
    echo "  TransportID \"trtype:PCIe traddr:${PCI_DEV_NAME}\" Nvme0" >> /tmp/spdk.bdev.conf
    ssh.push /tmp/spdk.bdev.conf /opt/aux/spdk.bdev.conf

    _cmd="LD_PRELOAD=${SPDK_FIOE_ROOT}/${ioengine_name} ${_cmd}"
    _cmd="${_cmd} --ioengine=${ioengine_name}"
    _cmd="${_cmd} --spdk_conf=${SPDK_FIOE_ROOT}/spdk.bdev.conf"
    _cmd="${_cmd} --filename=Nvme0n1"

  # Add the not-so-special-sauce for built-in io-engines
  else
    _cmd="${_cmd} --ioengine=${ioengine_name}"
    _cmd="${_cmd} --filename=${XNVME_URI}"
  fi

  if [[ -v FIO_AUX ]]; then
    _cmd="${_cmd} ${FIO_AUX}"
  fi

  local _trgt_fname="/tmp/fio-output.txt"

  _cmd="${_cmd} --output-format=normal,json --output=${_trgt_fname}"

  # Now run that fio tester!
  if ! cij.cmd "${_cmd}"; then
    cij.err "xnvme.fioe: error running fio"

    ssh.pull "${_trgt_fname}" "${output_fpath}"
    return 1
  fi

  # Download the output
  if ! ssh.pull "${_trgt_fname}" "${output_fpath}"; then
    cij.err "xnvme.fioe: failed pulling down fio output-file"
    return 0
  fi

  # Dump it to stdout for prosperity
  cat "${output_fpath}"

  return 0
}
