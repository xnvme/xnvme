#!/usr/bin/env bash
#
# This script is a concatenation of the "common.sh" and "setup.sh" scripts from
# the Intel SPDK project along with the PCI-ID definitions from . That is, the
# files in:
#
# SPDK/include/spdk/pci_ids.h
# SPDK/script/common.sh
# SPDK/script/setup.sh
#

PIDS="
#define SPDK_PCI_ANY_ID			0xffff
#define SPDK_PCI_VID_INTEL		0x8086
#define SPDK_PCI_VID_MEMBLAZE		0x1c5f
#define SPDK_PCI_VID_SAMSUNG		0x144d
#define SPDK_PCI_VID_VIRTUALBOX		0x80ee
#define SPDK_PCI_VID_VIRTIO		0x1af4
#define SPDK_PCI_VID_CNEXLABS		0x1d1d
#define SPDK_PCI_VID_VMWARE		0x15ad
#define SPDK_PCI_VID_REDHAT		0x1b36
#define SPDK_PCI_VID_NUTANIX		0x4e58
#define SPDK_PCI_VID_HUAWEI		0x19e5
#define SPDK_PCI_VID_MICROSOFT		0x1414

#define SPDK_PCI_CLASS_ANY_ID		0xffffff
/**
 * PCI class code for NVMe devices.
 *
 * Base class code 01h: mass storage
 * Subclass code 08h: non-volatile memory
 * Programming interface 02h: NVM Express
 */
#define SPDK_PCI_CLASS_NVME		0x010802

#define PCI_DEVICE_ID_INTEL_DSA		0x0b25
#define PCI_DEVICE_ID_INTEL_IAA		0x0cfe

#define PCI_DEVICE_ID_INTEL_IOAT_SNB0	0x3c20
#define PCI_DEVICE_ID_INTEL_IOAT_SNB1	0x3c21
#define PCI_DEVICE_ID_INTEL_IOAT_SNB2	0x3c22
#define PCI_DEVICE_ID_INTEL_IOAT_SNB3	0x3c23
#define PCI_DEVICE_ID_INTEL_IOAT_SNB4	0x3c24
#define PCI_DEVICE_ID_INTEL_IOAT_SNB5	0x3c25
#define PCI_DEVICE_ID_INTEL_IOAT_SNB6	0x3c26
#define PCI_DEVICE_ID_INTEL_IOAT_SNB7	0x3c27
#define PCI_DEVICE_ID_INTEL_IOAT_SNB8	0x3c2e
#define PCI_DEVICE_ID_INTEL_IOAT_SNB9	0x3c2f

#define PCI_DEVICE_ID_INTEL_IOAT_IVB0	0x0e20
#define PCI_DEVICE_ID_INTEL_IOAT_IVB1	0x0e21
#define PCI_DEVICE_ID_INTEL_IOAT_IVB2	0x0e22
#define PCI_DEVICE_ID_INTEL_IOAT_IVB3	0x0e23
#define PCI_DEVICE_ID_INTEL_IOAT_IVB4	0x0e24
#define PCI_DEVICE_ID_INTEL_IOAT_IVB5	0x0e25
#define PCI_DEVICE_ID_INTEL_IOAT_IVB6	0x0e26
#define PCI_DEVICE_ID_INTEL_IOAT_IVB7	0x0e27
#define PCI_DEVICE_ID_INTEL_IOAT_IVB8	0x0e2e
#define PCI_DEVICE_ID_INTEL_IOAT_IVB9	0x0e2f

#define PCI_DEVICE_ID_INTEL_IOAT_HSW0	0x2f20
#define PCI_DEVICE_ID_INTEL_IOAT_HSW1	0x2f21
#define PCI_DEVICE_ID_INTEL_IOAT_HSW2	0x2f22
#define PCI_DEVICE_ID_INTEL_IOAT_HSW3	0x2f23
#define PCI_DEVICE_ID_INTEL_IOAT_HSW4	0x2f24
#define PCI_DEVICE_ID_INTEL_IOAT_HSW5	0x2f25
#define PCI_DEVICE_ID_INTEL_IOAT_HSW6	0x2f26
#define PCI_DEVICE_ID_INTEL_IOAT_HSW7	0x2f27
#define PCI_DEVICE_ID_INTEL_IOAT_HSW8	0x2f2e
#define PCI_DEVICE_ID_INTEL_IOAT_HSW9	0x2f2f

#define PCI_DEVICE_ID_INTEL_IOAT_BWD0	0x0C50
#define PCI_DEVICE_ID_INTEL_IOAT_BWD1	0x0C51
#define PCI_DEVICE_ID_INTEL_IOAT_BWD2	0x0C52
#define PCI_DEVICE_ID_INTEL_IOAT_BWD3	0x0C53

#define PCI_DEVICE_ID_INTEL_IOAT_BDXDE0	0x6f50
#define PCI_DEVICE_ID_INTEL_IOAT_BDXDE1	0x6f51
#define PCI_DEVICE_ID_INTEL_IOAT_BDXDE2	0x6f52
#define PCI_DEVICE_ID_INTEL_IOAT_BDXDE3	0x6f53

#define PCI_DEVICE_ID_INTEL_IOAT_BDX0	0x6f20
#define PCI_DEVICE_ID_INTEL_IOAT_BDX1	0x6f21
#define PCI_DEVICE_ID_INTEL_IOAT_BDX2	0x6f22
#define PCI_DEVICE_ID_INTEL_IOAT_BDX3	0x6f23
#define PCI_DEVICE_ID_INTEL_IOAT_BDX4	0x6f24
#define PCI_DEVICE_ID_INTEL_IOAT_BDX5	0x6f25
#define PCI_DEVICE_ID_INTEL_IOAT_BDX6	0x6f26
#define PCI_DEVICE_ID_INTEL_IOAT_BDX7	0x6f27
#define PCI_DEVICE_ID_INTEL_IOAT_BDX8	0x6f2e
#define PCI_DEVICE_ID_INTEL_IOAT_BDX9	0x6f2f

#define PCI_DEVICE_ID_INTEL_IOAT_SKX	0x2021

#define PCI_DEVICE_ID_INTEL_IOAT_ICX	0x0b00

#define PCI_DEVICE_ID_VIRTIO_BLK_LEGACY	0x1001
#define PCI_DEVICE_ID_VIRTIO_SCSI_LEGACY 0x1004
#define PCI_DEVICE_ID_VIRTIO_BLK_MODERN	0x1042
#define PCI_DEVICE_ID_VIRTIO_SCSI_MODERN 0x1048

#define PCI_DEVICE_ID_VIRTIO_VHOST_USER 0x1017

#define PCI_DEVICE_ID_INTEL_VMD_SKX	0x201d
#define PCI_DEVICE_ID_INTEL_VMD_ICX	0x28c0

#define PCI_ROOT_PORT_A_INTEL_SKX	0x2030
#define PCI_ROOT_PORT_B_INTEL_SKX	0x2031
#define PCI_ROOT_PORT_C_INTEL_SKX	0x2032
#define PCI_ROOT_PORT_D_INTEL_SKX	0x2033
#define PCI_ROOT_PORT_A_INTEL_ICX	0x347a
#define PCI_ROOT_PORT_B_INTEL_ICX	0x347b
#define PCI_ROOT_PORT_C_INTEL_ICX	0x347c
#define PCI_ROOT_PORT_D_INTEL_ICX	0x347d
"

set -e
shopt -s nullglob extglob

os=$(uname -s)

if [[ $os != Linux && $os != FreeBSD ]]; then
	echo "Not supported platform ($os), aborting"
	exit 1
fi

rootdir=$(readlink -f $(dirname $0))/..

# Common shell utility functions

# Check if PCI device is in PCI_ALLOWED and not in PCI_BLOCKED
# Env:
# if PCI_ALLOWED is empty assume device is allowed
# if PCI_BLOCKED is empty assume device is NOT blocked
# Params:
# $1 - PCI BDF
function pci_can_use() {
	local i

	# The '\ ' part is important
	if [[ " $PCI_BLOCKED " =~ \ $1\  ]]; then
		return 1
	fi

	if [[ -z "$PCI_ALLOWED" ]]; then
		#no allow list specified, bind all devices
		return 0
	fi

	for i in $PCI_ALLOWED; do
		if [ "$i" == "$1" ]; then
			return 0
		fi
	done

	return 1
}

resolve_mod() {
	local mod=$1 aliases=()

	if aliases=($(modprobe -R "$mod")); then
		echo "${aliases[0]}"
	else
		echo "unknown"
	fi 2> /dev/null
}

cache_pci_init() {
	local -gA pci_bus_cache
	local -gA pci_ids_vendor
	local -gA pci_ids_device
	local -gA pci_bus_driver
	local -gA pci_mod_driver
	local -gA pci_mod_resolved

	[[ -z ${pci_bus_cache[*]} || $CMD == reset ]] || return 1

	pci_bus_cache=()
	pci_bus_ids_vendor=()
	pci_bus_ids_device=()
	pci_bus_driver=()
	pci_mod_driver=()
	pci_mod_resolved=()
}

cache_pci() {
	local pci=$1 class=$2 vendor=$3 device=$4 driver=$5 mod=$6

	if [[ -n $class ]]; then
		class=0x${class/0x/}
		pci_bus_cache["$class"]="${pci_bus_cache["$class"]:+${pci_bus_cache["$class"]} }$pci"
	fi
	if [[ -n $vendor && -n $device ]]; then
		vendor=0x${vendor/0x/} device=0x${device/0x/}
		pci_bus_cache["$vendor:$device"]="${pci_bus_cache["$vendor:$device"]:+${pci_bus_cache["$vendor:$device"]} }$pci"

		pci_ids_vendor["$pci"]=$vendor
		pci_ids_device["$pci"]=$device
	fi
	if [[ -n $driver ]]; then
		pci_bus_driver["$pci"]=$driver
	fi
	if [[ -n $mod ]]; then
		pci_mod_driver["$pci"]=$mod
		pci_mod_resolved["$pci"]=$(resolve_mod "$mod")
	fi
}

cache_pci_bus_sysfs() {
	[[ -e /sys/bus/pci/devices ]] || return 1

	cache_pci_init || return 0

	local pci
	local class vendor device driver mod

	for pci in /sys/bus/pci/devices/*; do
		class=$(< "$pci/class") vendor=$(< "$pci/vendor") device=$(< "$pci/device") driver="" mod=""
		if [[ -e $pci/driver ]]; then
			driver=$(readlink -f "$pci/driver")
			driver=${driver##*/}
		else
			driver=unbound
		fi
		if [[ -e $pci/modalias ]]; then
			mod=$(< "$pci/modalias")
		fi
		cache_pci "${pci##*/}" "$class" "$vendor" "$device" "$driver" "$mod"
	done
}

cache_pci_bus_lspci() {
	hash lspci 2> /dev/null || return 1

	cache_pci_init || return 0

	local dev
	while read -ra dev; do
		dev=("${dev[@]//\"/}")
		# lspci splits ls byte of the class (prog. interface) into a separate
		# field if it's != 0. Look for it and normalize the value to fit with
		# what kernel exposes under sysfs.
		if [[ ${dev[*]} =~ -p([0-9]+) ]]; then
			dev[1]+=${BASH_REMATCH[1]}
		else
			dev[1]+=00
		fi
		# pci class vendor device
		cache_pci "${dev[@]::4}"
	done < <(lspci -Dnmm)
}

cache_pci_bus_pciconf() {
	hash pciconf 2> /dev/null || return 1

	cache_pci_init || return 0

	local class vendor device
	local pci pci_info
	local chip driver

	while read -r pci pci_info; do
		driver=${pci%@*}
		pci=${pci##*pci} pci=${pci%:}
		source <(echo "$pci_info")
		# pciconf under FreeBSD 13.1 provides vendor and device IDs in its
		# output under separate, dedicated fields. For 12.x they need to
		# be extracted from the chip field.
		if [[ -n $chip ]]; then
			vendor=$(printf '0x%04x' $((chip & 0xffff)))
			device=$(printf '0x%04x' $(((chip >> 16) & 0xffff)))
		fi
		cache_pci "$pci" "$class" "$vendor" "$device" "$driver"
	done < <(pciconf -l)
}

cache_pci_bus() {
	case "$(uname -s)" in
		Linux) cache_pci_bus_lspci || cache_pci_bus_sysfs ;;
		FreeBSD) cache_pci_bus_pciconf ;;
	esac
}

iter_all_pci_sysfs() {
	cache_pci_bus_sysfs || return 1

	# default to class of the nvme devices
	local find=${1:-0x010802} findx=$2
	local pci pcis

	[[ -n ${pci_bus_cache["$find"]} ]] || return 0
	read -ra pcis <<< "${pci_bus_cache["$find"]}"

	if ((findx)); then
		printf '%s\n' "${pcis[@]::findx}"
	else
		printf '%s\n' "${pcis[@]}"
	fi
}

# This function will ignore PCI PCI_ALLOWED and PCI_BLOCKED
function iter_all_pci_class_code() {
	local class
	local subclass
	local progif
	class="$(printf %02x $((0x$1)))"
	subclass="$(printf %02x $((0x$2)))"
	progif="$(printf %02x $((0x$3)))"

	if hash lspci &> /dev/null; then
		if [ "$progif" != "00" ]; then
			lspci -mm -n -D \
				| grep -i -- "-p${progif}" \
				| awk -v cc="\"${class}${subclass}\"" -F " " \
					'{if (cc ~ $2) print $1}' | tr -d '"'
		else
			lspci -mm -n -D \
				| awk -v cc="\"${class}${subclass}\"" -F " " \
					'{if (cc ~ $2) print $1}' | tr -d '"'
		fi
	elif hash pciconf &> /dev/null; then
		local addr=($(pciconf -l | grep -i "class=0x${class}${subclass}${progif}" \
			| cut -d$'\t' -f1 | sed -e 's/^[a-zA-Z0-9_]*@pci//g' | tr ':' ' '))
		echo "${addr[0]}:${addr[1]}:${addr[2]}:${addr[3]}"
	elif iter_all_pci_sysfs "$(printf '0x%06x' $((0x$progif | 0x$subclass << 8 | 0x$class << 16)))"; then
		:
	else
		echo "Missing PCI enumeration utility" >&2
		exit 1
	fi
}

# This function will ignore PCI PCI_ALLOWED and PCI_BLOCKED
function iter_all_pci_dev_id() {
	local ven_id
	local dev_id
	ven_id="$(printf %04x $((0x$1)))"
	dev_id="$(printf %04x $((0x$2)))"

	if hash lspci &> /dev/null; then
		lspci -mm -n -D | awk -v ven="\"$ven_id\"" -v dev="\"${dev_id}\"" -F " " \
			'{if (ven ~ $3 && dev ~ $4) print $1}' | tr -d '"'
	elif hash pciconf &> /dev/null; then
		local addr=($(pciconf -l | grep -iE "chip=0x${dev_id}${ven_id}|vendor=0x$ven_id device=0x$dev_id" \
			| cut -d$'\t' -f1 | sed -e 's/^[a-zA-Z0-9_]*@pci//g' | tr ':' ' '))
		echo "${addr[0]}:${addr[1]}:${addr[2]}:${addr[3]}"
	elif iter_all_pci_sysfs "0x$ven_id:0x$dev_id"; then
		:
	else
		echo "Missing PCI enumeration utility" >&2
		exit 1
	fi
}

function iter_pci_dev_id() {
	local bdf=""

	for bdf in $(iter_all_pci_dev_id "$@"); do
		if pci_can_use "$bdf"; then
			echo "$bdf"
		fi
	done
}

# This function will filter out PCI devices using PCI_ALLOWED and PCI_BLOCKED
# See function pci_can_use()
function iter_pci_class_code() {
	local bdf=""

	for bdf in $(iter_all_pci_class_code "$@"); do
		if pci_can_use "$bdf"; then
			echo "$bdf"
		fi
	done
}

function nvme_in_userspace() {
	# Check used drivers. If it's not vfio-pci or uio-pci-generic
	# then most likely PCI_ALLOWED option was used for setup.sh
	# and we do not want to use that disk.

	local bdf bdfs
	local nvmes

	if [[ -n ${pci_bus_cache["0x010802"]} ]]; then
		nvmes=(${pci_bus_cache["0x010802"]})
	else
		nvmes=($(iter_pci_class_code 01 08 02))
	fi

	for bdf in "${nvmes[@]}"; do
		if [[ -e /sys/bus/pci/drivers/nvme/$bdf ]] \
			|| [[ $(uname -s) == FreeBSD && $(pciconf -l "pci${bdf/./:}") == nvme* ]]; then
			continue
		fi
		bdfs+=("$bdf")
	done
	((${#bdfs[@]})) || return 1
	printf '%s\n' "${bdfs[@]}"
}

cmp_versions() {
	local ver1 ver1_l
	local ver2 ver2_l

	IFS=".-:" read -ra ver1 <<< "$1"
	IFS=".-:" read -ra ver2 <<< "$3"
	local op=$2

	ver1_l=${#ver1[@]}
	ver2_l=${#ver2[@]}

	local lt=0 gt=0 eq=0 v
	case "$op" in
		"<") : $((eq = gt = 1)) ;;
		">") : $((eq = lt = 1)) ;;
		"<=") : $((gt = 1)) ;;
		">=") : $((lt = 1)) ;;
		"==") : $((lt = gt = 1)) ;;
	esac

	decimal() (
		local d=${1,,}
		if [[ $d =~ ^[0-9]+$ ]]; then
			echo $((10#$d))
		elif [[ $d =~ ^0x || $d =~ ^[a-f0-9]+$ ]]; then
			d=${d/0x/}
			echo $((0x$d))
		else
			echo 0
		fi
	)

	for ((v = 0; v < (ver1_l > ver2_l ? ver1_l : ver2_l); v++)); do
		ver1[v]=$(decimal "${ver1[v]}")
		ver2[v]=$(decimal "${ver2[v]}")
		((ver1[v] > ver2[v])) && return "$gt"
		((ver1[v] < ver2[v])) && return "$lt"
	done
	[[ ${ver1[*]} == "${ver2[*]}" ]] && return "$eq"
}

lt() { cmp_versions "$1" "<" "$2"; }
gt() { cmp_versions "$1" ">" "$2"; }
le() { cmp_versions "$1" "<=" "$2"; }
ge() { cmp_versions "$1" ">=" "$2"; }
eq() { cmp_versions "$1" "==" "$2"; }
neq() { ! eq "$1" "$2"; }

block_in_use() {
	local block=$1 data pt
	# Skip devices that are in use - simple blkid it to see if
	# there's any metadata (pt, fs, etc.) present on the drive.
	# FIXME: Special case to ignore atari as a potential false
	# positive:
	# https://github.com/spdk/spdk/issues/2079
	# Devices with SPDK's GPT part type are not considered to
	# be in use.

	if "$rootdir/scripts/spdk-gpt.py" "$block"; then
		return 1
	fi

	data=$(blkid "/dev/${block##*/}") || data=none

	if [[ $data == none ]]; then
		return 1
	fi

	pt=$(blkid -s PTTYPE -o value "/dev/${block##*/}") || pt=none

	if [[ $pt == none || $pt == atari ]]; then
		return 1
	fi

	# Devices used in SPDK tests always create GPT partitions
	# with label containing SPDK_TEST string. Such devices were
	# part of the tests before, so are not considered in use.
	if [[ $pt == gpt ]] && parted "/dev/${block##*/}" -ms print | grep -q "SPDK_TEST"; then
		return 1
	fi

	return 0
}

get_spdk_gpt() {
	local spdk_guid

	[[ -e $rootdir/module/bdev/gpt/gpt.h ]] || return 1

	IFS="()" read -r _ spdk_guid _ < <(grep SPDK_GPT_PART_TYPE_GUID "$rootdir/module/bdev/gpt/gpt.h")
	spdk_guid=${spdk_guid//, /-} spdk_guid=${spdk_guid//0x/}

	echo "$spdk_guid"
}

if [[ -e "$CONFIG_WPDK_DIR/bin/wpdk_common.sh" ]]; then
	# Adjust uname to report the operating system as WSL, Msys or Cygwin
	# and the kernel name as Windows. Define kill() to invoke the SIGTERM
	# handler before causing a hard stop with TerminateProcess.
	source "$CONFIG_WPDK_DIR/bin/wpdk_common.sh"
fi


function usage() {
	if [[ $os == Linux ]]; then
		options="[config|reset|status|cleanup|help]"
	else
		options="[config|reset|help]"
	fi

	[[ -n $2 ]] && (
		echo "$2"
		echo ""
	)
	echo "Helper script for allocating hugepages and binding NVMe, I/OAT, VMD and Virtio devices"
	echo "to a generic VFIO kernel driver. If VFIO is not available on the system, this script"
	echo "will fall back to UIO. NVMe and Virtio devices with active mountpoints will be ignored."
	echo "All hugepage operations use default hugepage size on the system (hugepagesz)."
	echo "Usage: $(basename $1) $options"
	echo
	echo "$options - as following:"
	echo "config            Default mode. Allocate hugepages and bind PCI devices."
	if [[ $os == Linux ]]; then
		echo "cleanup           Remove any orphaned files that can be left in the system after SPDK application exit"
	fi
	echo "reset             Rebind PCI devices back to their original drivers."
	echo "                  Also cleanup any leftover spdk files/resources."
	echo "                  Hugepage memory size will remain unchanged."
	if [[ $os == Linux ]]; then
		echo "status            Print status of all SPDK-compatible devices on the system."
	fi
	echo "help              Print this help message."
	echo
	echo "The following environment variables can be specified."
	echo "HUGEMEM           Size of hugepage memory to allocate (in MB). 2048 by default."
	echo "                  For NUMA systems, the hugepages will be distributed on node0 by"
	echo "                  default."
	echo "HUGE_EVEN_ALLOC   If set to 'yes', hugepages will be evenly distributed across all"
	echo "                  system's NUMA nodes (effectively ignoring anything set in HUGENODE)."
	echo "                  Uses kernel's default for hugepages size."
	echo "NRHUGE            Number of hugepages to allocate. This variable overwrites HUGEMEM."
	echo "HUGENODE          Specific NUMA node to allocate hugepages on. Multiple nodes can be"
	echo "                  separated with comas. By default, NRHUGE will be applied on each node."
	echo "                  Hugepages can be defined per node with e.g.:"
	echo "                  HUGENODE='nodes_hp[0]=2048,nodes_hp[1]=512,2' - this will allocate"
	echo "                  2048 pages for node0, 512 for node1 and default NRHUGE for node2."
	echo "HUGEPGSZ          Size of the hugepages to use in kB. If not set, kernel's default"
	echo "                  setting is used."
	echo "SHRINK_HUGE       If set to 'yes', hugepages allocation won't be skipped in case"
	echo "                  number of requested hugepages is lower from what's already"
	echo "                  allocated. Doesn't apply when HUGE_EVEN_ALLOC is in use."
	echo "CLEAR_HUGE        If set to 'yes', the attempt to remove hugepages from all nodes will"
	echo "                  be made prior to allocation".
	echo "PCI_ALLOWED"
	echo "PCI_BLOCKED       Whitespace separated list of PCI devices (NVMe, I/OAT, VMD, Virtio)."
	echo "                  Each device must be specified as a full PCI address."
	echo "                  E.g. PCI_ALLOWED=\"0000:01:00.0 0000:02:00.0\""
	echo "                  To block all PCI devices: PCI_ALLOWED=\"none\""
	echo "                  To allow all PCI devices except 0000:01:00.0: PCI_BLOCKED=\"0000:01:00.0\""
	echo "                  To allow only PCI device 0000:01:00.0: PCI_ALLOWED=\"0000:01:00.0\""
	echo "                  If PCI_ALLOWED and PCI_BLOCKED are empty or unset, all PCI devices"
	echo "                  will be bound."
	echo "                  Each device in PCI_BLOCKED will be ignored (driver won't be changed)."
	echo "                  PCI_BLOCKED has precedence over PCI_ALLOWED."
	echo "TARGET_USER       User that will own hugepage mountpoint directory and vfio groups."
	echo "                  By default the current user will be used."
	echo "DRIVER_OVERRIDE   Disable automatic vfio-pci/uio_pci_generic selection and forcefully"
	echo "                  bind devices to the given driver."
	echo "                  E.g. DRIVER_OVERRIDE=uio_pci_generic or DRIVER_OVERRIDE=/home/public/dpdk/build/kmod/igb_uio.ko"
	echo "PCI_BLOCK_SYNC_ON_RESET"
	echo "                  If set in the environment, the attempt to wait for block devices associated"
	echo "                  with given PCI device will be made upon reset"
	exit 0
}

# In monolithic kernels the lsmod won't work. So
# back that with a /sys/modules. We also check
# /sys/bus/pci/drivers/ as neither lsmod nor /sys/modules might
# contain needed info (like in Fedora-like OS).
function check_for_driver() {
	if [[ -z $1 ]]; then
		return 0
	fi

	if lsmod | grep -q ${1//-/_}; then
		return 1
	fi

	if [[ -d /sys/module/${1} || -d \
		/sys/module/${1//-/_} || -d \
		/sys/bus/pci/drivers/${1} || -d \
		/sys/bus/pci/drivers/${1//-/_} ]]; then
		return 2
	fi
	return 0
}

function check_for_driver_freebsd() {
	# Check if dpdk drivers (nic_uio, contigmem) are in the kernel's module path.
	local search_paths path driver
	IFS=";" read -ra search_paths < <(kldconfig -rU)

	for driver in contigmem.ko nic_uio.ko; do
		for path in "${search_paths[@]}"; do
			[[ -f $path/$driver ]] && continue 2
		done
		return 1
	done
	return 0
}

function pci_dev_echo() {
	local bdf="$1"
	shift
	echo "$bdf (${pci_ids_vendor["$bdf"]#0x} ${pci_ids_device["$bdf"]#0x}): $*"
}

function linux_bind_driver() {
	bdf="$1"
	driver_name="$2"
	old_driver_name=${drivers_d["$bdf"]:-no driver}
	ven_dev_id="${pci_ids_vendor["$bdf"]#0x} ${pci_ids_device["$bdf"]#0x}"

	if [[ $driver_name == "$old_driver_name" ]]; then
		pci_dev_echo "$bdf" "Already using the $old_driver_name driver"
		return 0
	fi

	if [[ $old_driver_name != "no driver" ]]; then
		echo "$ven_dev_id" > "/sys/bus/pci/devices/$bdf/driver/remove_id" 2> /dev/null || true
		echo "$bdf" > "/sys/bus/pci/devices/$bdf/driver/unbind"
	fi

	pci_dev_echo "$bdf" "$old_driver_name -> $driver_name"

	if [[ $driver_name == "none" ]]; then
		return 0
	fi

	echo "$ven_dev_id" > "/sys/bus/pci/drivers/$driver_name/new_id" 2> /dev/null || true
	echo "$bdf" > "/sys/bus/pci/drivers/$driver_name/bind" 2> /dev/null || true

	if [[ $driver_name == uio_pci_generic ]] && ! check_for_driver igb_uio; then
		# Check if the uio_pci_generic driver is broken as it might be in
		# some 4.18.x kernels (see centos8 for instance) - if our device
		# didn't get a proper uio entry, fallback to igb_uio
		if [[ ! -e /sys/bus/pci/devices/$bdf/uio ]]; then
			pci_dev_echo "$bdf" "uio_pci_generic potentially broken, moving to igb_uio"
			drivers_d["$bdf"]="no driver"
			# This call will override $driver_name for remaining devices as well
			linux_bind_driver "$bdf" igb_uio
		fi
	fi

	iommu_group=$(basename $(readlink -f /sys/bus/pci/devices/$bdf/iommu_group))
	if [ -e "/dev/vfio/$iommu_group" ]; then
		if [ -n "$TARGET_USER" ]; then
			chown "$TARGET_USER" "/dev/vfio/$iommu_group"
		fi
	fi
}

function linux_unbind_driver() {
	local bdf="$1"
	local ven_dev_id
	ven_dev_id="${pci_ids_vendor["$bdf"]#0x} ${pci_ids_device["$bdf"]#0x}"
	local old_driver_name=${drivers_d["$bdf"]:-no driver}

	if [[ $old_driver_name == "no driver" ]]; then
		pci_dev_echo "$bdf" "Not bound to any driver"
		return 0
	fi

	if [[ -e /sys/bus/pci/drivers/$old_driver_name ]]; then
		echo "$ven_dev_id" > "/sys/bus/pci/drivers/$old_driver_name/remove_id" 2> /dev/null || true
		echo "$bdf" > "/sys/bus/pci/drivers/$old_driver_name/unbind"
	fi

	pci_dev_echo "$bdf" "$old_driver_name -> no driver"
}

function linux_hugetlbfs_mounts() {
	mount | grep ' type hugetlbfs ' | awk '{ print $3 }'
}

function get_block_dev_from_bdf() {
	local bdf=$1
	local block

	for block in /sys/block/*; do
		if [[ $(readlink -f "$block/device") == *"/$bdf/"* ]]; then
			echo "${block##*/}"
		fi
	done
}

function get_used_bdf_block_devs() {
	local bdf=$1
	local blocks block blockp dev mount holder
	local used

	hash lsblk &> /dev/null || return 1
	blocks=($(get_block_dev_from_bdf "$bdf"))

	for block in "${blocks[@]}"; do
		# Check if the device is hold by some other, regardless if it's mounted
		# or not.
		for holder in "/sys/class/block/$block"*/holders/*; do
			[[ -e $holder ]] || continue
			blockp=${holder%/holders*} blockp=${blockp##*/}
			if [[ -e $holder/slaves/$blockp ]]; then
				used+=("holder@$blockp:${holder##*/}")
			fi
		done
		while read -r dev mount; do
			if [[ -e $mount ]]; then
				used+=("mount@$block:$dev")
			fi
		done < <(lsblk -l -n -o NAME,MOUNTPOINT "/dev/$block")
		if ((${#used[@]} == 0)); then
			# Make sure we check if there's any valid data present on the target device
			# regardless if it's being actively used or not. This is mainly done to make
			# sure we don't miss more complex setups like ZFS pools, etc.
			if block_in_use "$block" > /dev/null; then
				used+=("data@$block")
			fi
		fi
	done

	if ((${#used[@]} > 0)); then
		printf '%s\n' "${used[@]}"
	fi
}

function collect_devices() {
	# NVMe, IOAT, DSA, IAA, VIRTIO, VMD

	local ids dev_type dev_id bdf bdfs in_use driver

	ids+="PCI_DEVICE_ID_INTEL_IOAT"
	ids+="|PCI_DEVICE_ID_INTEL_DSA"
	ids+="|PCI_DEVICE_ID_INTEL_IAA"
	ids+="|PCI_DEVICE_ID_VIRTIO"
	ids+="|PCI_DEVICE_ID_INTEL_VMD"
	ids+="|SPDK_PCI_CLASS_NVME"

	local -gA nvme_d ioat_d dsa_d iaa_d virtio_d vmd_d all_devices_d drivers_d

	while read -r _ dev_type dev_id; do
		bdfs=(${pci_bus_cache["0x8086:$dev_id"]})
		[[ $dev_type == *NVME* ]] && bdfs=(${pci_bus_cache["$dev_id"]})
		[[ $dev_type == *VIRT* ]] && bdfs=(${pci_bus_cache["0x1af4:$dev_id"]})
		[[ $dev_type =~ (NVME|IOAT|DSA|IAA|VIRTIO|VMD) ]] && dev_type=${BASH_REMATCH[1],,}
		for bdf in "${bdfs[@]}"; do
			in_use=0
			if [[ $1 != status ]]; then
				if ! pci_can_use "$bdf"; then
					pci_dev_echo "$bdf" "Skipping denied controller at $bdf"
					in_use=1
				fi
				if [[ $dev_type == nvme || $dev_type == virtio ]]; then
					if ! verify_bdf_block_devs "$bdf"; then
						in_use=1
					fi
				fi
				if [[ $dev_type == vmd ]]; then
					if [[ $PCI_ALLOWED != *"$bdf"* ]]; then
						pci_dev_echo "$bdf" "Skipping not allowed VMD controller at $bdf"
						in_use=1
					elif [[ " ${drivers_d[*]} " =~ "nvme" ]]; then
						if [[ "${DRIVER_OVERRIDE}" != "none" ]]; then
							if [ "$mode" == "config" ]; then
								cat <<- MESSAGE
									Binding new driver to VMD device. If there are NVMe SSDs behind the VMD endpoint
									which are attached to the kernel NVMe driver,the binding process may go faster
									if you first run this script with DRIVER_OVERRIDE="none" to unbind only the
									NVMe SSDs, and then run again to unbind the VMD devices."
								MESSAGE
							fi
						fi
					fi
				fi
			fi
			eval "${dev_type}_d[$bdf]=$in_use"
			all_devices_d["$bdf"]=$in_use
			if [[ -e /sys/bus/pci/devices/$bdf/driver ]]; then
				driver=$(readlink -f "/sys/bus/pci/devices/$bdf/driver")
				drivers_d["$bdf"]=${driver##*/}
			fi
		done
	done < <(echo "$PIDS" | grep -E "$ids")
}

function collect_driver() {
	local bdf=$1
	local drivers driver

	if [[ -e /sys/bus/pci/devices/$bdf/modalias ]] \
		&& drivers=($(modprobe -R "$(< "/sys/bus/pci/devices/$bdf/modalias")")); then
		# Pick first entry in case multiple aliases are bound to a driver.
		driver=$(readlink -f "/sys/module/${drivers[0]}/drivers/pci:"*)
		driver=${driver##*/}
	else
		[[ -n ${nvme_d["$bdf"]} ]] && driver=nvme
		[[ -n ${ioat_d["$bdf"]} ]] && driver=ioatdma
		[[ -n ${dsa_d["$bdf"]} ]] && driver=dsa
		[[ -n ${iaa_d["$bdf"]} ]] && driver=iaa
		[[ -n ${virtio_d["$bdf"]} ]] && driver=virtio-pci
		[[ -n ${vmd_d["$bdf"]} ]] && driver=vmd
	fi 2> /dev/null
	echo "$driver"
}

function verify_bdf_block_devs() {
	local bdf=$1
	local blknames
	blknames=($(get_used_bdf_block_devs "$bdf")) || return 1

	if ((${#blknames[@]} > 0)); then
		local IFS=","
		pci_dev_echo "$bdf" "Active devices: ${blknames[*]}, so not binding PCI dev"
		return 1
	fi
}

function configure_linux_pci() {
	local driver_path=""
	driver_name=""
	igb_uio_fallback=""

	if [[ -r "$rootdir/dpdk/build-tmp/kernel/linux/igb_uio/igb_uio.ko" ]]; then
		# igb_uio is a common driver to override with and it depends on uio.
		modprobe uio || true
		if ! check_for_driver igb_uio || insmod "$rootdir/dpdk/build-tmp/kernel/linux/igb_uio/igb_uio.ko"; then
			igb_uio_fallback="$rootdir/dpdk/build-tmp/kernel/linux/igb_uio/igb_uio.ko"
		fi
	fi

	if [[ "${DRIVER_OVERRIDE}" == "none" ]]; then
		driver_name=none
	elif [[ -n "${DRIVER_OVERRIDE}" ]]; then
		driver_path="$DRIVER_OVERRIDE"
		driver_name="${DRIVER_OVERRIDE##*/}"
		# modprobe and the sysfs don't use the .ko suffix.
		driver_name=${driver_name%.ko}
		# path = name -> there is no path
		if [[ "$driver_path" = "$driver_name" ]]; then
			driver_path=""
		fi
	elif [[ -n "$(ls /sys/kernel/iommu_groups)" || (-e \
	/sys/module/vfio/parameters/enable_unsafe_noiommu_mode && \
	"$(cat /sys/module/vfio/parameters/enable_unsafe_noiommu_mode)" == "Y") ]]; then
		driver_name=vfio-pci
		# Just in case, attempt to load VFIO_IOMMU_TYPE1 module into the kernel - this
		# should be done automatically by modprobe since this particular module should
		# be a part of vfio-pci dependencies, however, on some distros, it seems that
		# it's not the case. See #1689.
		if modinfo vfio_iommu_type1 > /dev/null; then
			modprobe vfio_iommu_type1
		fi
	elif ! check_for_driver uio_pci_generic || modinfo uio_pci_generic > /dev/null 2>&1; then
		driver_name=uio_pci_generic
	elif [[ -e $igb_uio_fallback ]]; then
		driver_path="$igb_uio_fallback"
		driver_name="igb_uio"
		echo "WARNING: uio_pci_generic not detected - using $driver_name"
	else
		echo "No valid drivers found [vfio-pci, uio_pci_generic, igb_uio]. Please enable one of the kernel modules."
		return 1
	fi

	# modprobe assumes the directory of the module. If the user passes in a path, we should use insmod
	if [[ $driver_name != "none" ]]; then
		if [[ -n "$driver_path" ]]; then
			insmod $driver_path || true
		else
			modprobe $driver_name
		fi
	fi

	for bdf in "${!all_devices_d[@]}"; do
		if ((all_devices_d["$bdf"] == 0)); then
			if [[ -n ${nvme_d["$bdf"]} ]]; then
				# Some nvme controllers may take significant amount of time while being
				# unbound from the driver. Put that task into background to speed up the
				# whole process. Currently this is done only for the devices bound to the
				# nvme driver as other, i.e., ioatdma's, trigger a kernel BUG when being
				# unbound in parallel. See https://bugzilla.kernel.org/show_bug.cgi?id=209041.
				linux_bind_driver "$bdf" "$driver_name" &
			else
				linux_bind_driver "$bdf" "$driver_name"
			fi
		fi
	done
	wait

	echo "1" > "/sys/bus/pci/rescan"
}

function cleanup_linux() {
	local dirs_to_clean=() files_to_clean=() opened_files=() file_locks=()
	local match_spdk="spdk_tgt|iscsi|vhost|nvmf|rocksdb|bdevio|bdevperf|vhost_fuzz|nvme_fuzz|accel_perf|bdev_svc"

	dirs_to_clean=({/var/run,/tmp}/dpdk/spdk{,_pid}+([0-9]))
	if [[ -d $XDG_RUNTIME_DIR ]]; then
		dirs_to_clean+=("$XDG_RUNTIME_DIR/dpdk/spdk"{,_pid}+([0-9]))
	fi

	for dir in "${dirs_to_clean[@]}"; do
		files_to_clean+=("$dir/"*)
	done
	file_locks+=(/var/tmp/spdk_pci_lock*)

	files_to_clean+=(/dev/shm/@(@($match_spdk)_trace|spdk_iscsi_conns))
	files_to_clean+=("${file_locks[@]}")

	# This may fail in case path that readlink attempts to resolve suddenly
	# disappears (as it may happen with terminating processes).
	opened_files+=($(readlink -f /proc/+([0-9])/fd/+([0-9]))) || true

	if ((${#opened_files[@]} == 0)); then
		echo "Can't get list of opened files!"
		exit 1
	fi

	echo 'Cleaning'
	for f in "${files_to_clean[@]}"; do
		[[ -e $f ]] || continue
		if [[ ${opened_files[*]} != *"$f"* ]]; then
			echo "Removing:    $f"
			rm $f
		else
			echo "Still open: $f"
		fi
	done

	for dir in "${dirs_to_clean[@]}"; do
		[[ -d $dir ]] || continue
		if [[ ${opened_files[*]} != *"$dir"* ]]; then
			echo "Removing:    $dir"
			rmdir $dir
		else
			echo "Still open: $dir"
		fi
	done
	echo "Clean"
}

check_hugepages_alloc() {
	local hp_int=$1
	local allocated_hugepages

	allocated_hugepages=$(< "$hp_int")

	if ((NRHUGE <= allocated_hugepages)) && [[ $SHRINK_HUGE != yes ]]; then
		echo "INFO: Requested $NRHUGE hugepages but $allocated_hugepages already allocated ${2:+on node$2}"
		return 0
	fi

	echo $((NRHUGE < 0 ? 0 : NRHUGE)) > "$hp_int"

	allocated_hugepages=$(< "$hp_int")
	if ((allocated_hugepages < NRHUGE)); then
		cat <<- ERROR

			## ERROR: requested $NRHUGE hugepages but $allocated_hugepages could be allocated ${2:+on node$2}.
			## Memory might be heavily fragmented. Please try flushing the system cache, or reboot the machine.
		ERROR
		return 1
	fi
}

clear_hugepages() { echo 0 > /proc/sys/vm/nr_hugepages; }

configure_linux_hugepages() {
	local node system_nodes
	local nodes_to_use nodes_hp

	if [[ $CLEAR_HUGE == yes ]]; then
		clear_hugepages
	fi

	if [[ $HUGE_EVEN_ALLOC == yes ]]; then
		clear_hugepages
		check_hugepages_alloc /proc/sys/vm/nr_hugepages
		return 0
	fi

	for node in /sys/devices/system/node/node*; do
		[[ -e $node ]] || continue
		nodes[${node##*node}]=$node/hugepages/hugepages-${HUGEPGSZ}kB/nr_hugepages
	done

	if ((${#nodes[@]} == 0)); then
		# No NUMA support? Fallback to common interface
		check_hugepages_alloc /proc/sys/vm/nr_hugepages
		return 0
	fi

	IFS="," read -ra nodes_to_use <<< "$HUGENODE"
	if ((${#nodes_to_use[@]} == 0)); then
		nodes_to_use[0]=0
	fi

	# Align indexes with node ids
	for node in "${!nodes_to_use[@]}"; do
		if [[ ${nodes_to_use[node]} =~ ^nodes_hp\[[0-9]+\]= ]]; then
			eval "${nodes_to_use[node]}"
		elif [[ ${nodes_to_use[node]} =~ ^[0-9]+$ ]]; then
			nodes_hp[nodes_to_use[node]]=$NRHUGE
		fi
	done

	for node in "${!nodes_hp[@]}"; do
		if [[ -z ${nodes[node]} ]]; then
			echo "Node $node doesn't exist, ignoring" >&2
			continue
		fi
		NRHUGE=${nodes_hp[node]:-$NRHUGE} check_hugepages_alloc "${nodes[node]}" "$node"
	done
}

function configure_linux() {
	configure_linux_pci
	hugetlbfs_mounts=$(linux_hugetlbfs_mounts)

	if [ -z "$hugetlbfs_mounts" ]; then
		hugetlbfs_mounts=/mnt/huge
		echo "Mounting hugetlbfs at $hugetlbfs_mounts"
		mkdir -p "$hugetlbfs_mounts"
		mount -t hugetlbfs nodev "$hugetlbfs_mounts"
	fi

	configure_linux_hugepages

	if [ "$driver_name" = "vfio-pci" ]; then
		if [ -n "$TARGET_USER" ]; then
			for mount in $hugetlbfs_mounts; do
				chown "$TARGET_USER" "$mount"
				chmod g+w "$mount"
			done

			MEMLOCK_AMNT=$(su "$TARGET_USER" -c "ulimit -l")
			if [[ $MEMLOCK_AMNT != "unlimited" ]]; then
				MEMLOCK_MB=$((MEMLOCK_AMNT / 1024))
				cat <<- MEMLOCK
					"$TARGET_USER" user memlock limit: $MEMLOCK_MB MB

					This is the maximum amount of memory you will be
					able to use with DPDK and VFIO if run as user "$TARGET_USER".
					To change this, please adjust limits.conf memlock limit for user "$TARGET_USER".
				MEMLOCK
				if ((MEMLOCK_AMNT < 65536)); then
					echo ""
					echo "## WARNING: memlock limit is less than 64MB"
					echo -n "## DPDK with VFIO may not be able to initialize "
					echo "if run as user \"$TARGET_USER\"."
				fi
			fi
		fi
	fi

	if [ $(uname -i) == "x86_64" ] && [ ! -e /dev/cpu/0/msr ]; then
		# Some distros build msr as a module.  Make sure it's loaded to ensure
		#  DPDK can easily figure out the TSC rate rather than relying on 100ms
		#  sleeps.
		modprobe msr &> /dev/null || true
	fi
}

function reset_linux_pci() {
	# virtio
	# TODO: check if virtio-pci is loaded first and just unbind if it is not loaded
	# Requires some more investigation - for example, some kernels do not seem to have
	#  virtio-pci but just virtio_scsi instead.  Also need to make sure we get the
	#  underscore vs. dash right in the virtio_scsi name.
	modprobe virtio-pci || true
	for bdf in "${!all_devices_d[@]}"; do
		((all_devices_d["$bdf"] == 0)) || continue

		driver=$(collect_driver "$bdf")
		if [[ -n $driver ]] && ! check_for_driver "$driver"; then
			linux_bind_driver "$bdf" "$driver"
		else
			linux_unbind_driver "$bdf"
		fi
	done

	echo "1" > "/sys/bus/pci/rescan"
}

function reset_linux() {
	reset_linux_pci
	for mount in $(linux_hugetlbfs_mounts); do
		for hp in "$mount"/spdk*map_*; do
			flock -n "$hp" true && rm -f "$hp"
		done
	done
	rm -f /run/.spdk*
}

function status_linux() {
	echo "Hugepages" >&2
	printf "%-6s %10s %8s / %6s\n" "node" "hugesize" "free" "total" >&2

	numa_nodes=0
	for path in /sys/devices/system/node/node*/hugepages/hugepages-*/; do
		numa_nodes=$((numa_nodes + 1))
		free_pages=$(cat $path/free_hugepages)
		all_pages=$(cat $path/nr_hugepages)

		[[ $path =~ (node[0-9]+)/hugepages/hugepages-([0-9]+kB) ]]

		node=${BASH_REMATCH[1]}
		huge_size=${BASH_REMATCH[2]}

		printf "%-6s %10s %8s / %6s\n" $node $huge_size $free_pages $all_pages
	done

	# fall back to system-wide hugepages
	if [ "$numa_nodes" = "0" ]; then
		free_pages=$(grep HugePages_Free /proc/meminfo | awk '{ print $2 }')
		all_pages=$(grep HugePages_Total /proc/meminfo | awk '{ print $2 }')
		node="-"
		huge_size="$HUGEPGSZ"

		printf "%-6s %10s %8s / %6s\n" $node $huge_size $free_pages $all_pages
	fi

	printf '\n%-8s %-15s %-6s %-6s %-7s %-16s %-10s %s\n' \
		"Type" "BDF" "Vendor" "Device" "NUMA" "Driver" "Device" "Block devices" >&2

	sorted_bdfs=($(printf '%s\n' "${!all_devices_d[@]}" | sort))

	for bdf in "${sorted_bdfs[@]}"; do
		driver=${drivers_d["$bdf"]}
		if [ "$numa_nodes" = "0" ]; then
			node="-"
		else
			node=$(cat /sys/bus/pci/devices/$bdf/numa_node)
			if ((node == -1)); then
				node=unknown
			fi
		fi
		if [ "$driver" = "nvme" ] && [ -d /sys/bus/pci/devices/$bdf/nvme ]; then
			name=$(ls /sys/bus/pci/devices/$bdf/nvme)
		else
			name="-"
		fi

		if [[ -n ${nvme_d["$bdf"]} || -n ${virtio_d["$bdf"]} ]]; then
			blknames=($(get_block_dev_from_bdf "$bdf"))
		else
			blknames=("-")
		fi

		desc=""
		desc=${desc:-${nvme_d["$bdf"]:+NVMe}}
		desc=${desc:-${ioat_d["$bdf"]:+I/OAT}}
		desc=${desc:-${dsa_d["$bdf"]:+DSA}}
		desc=${desc:-${iaa_d["$bdf"]:+IAA}}
		desc=${desc:-${virtio_d["$bdf"]:+virtio}}
		desc=${desc:-${vmd_d["$bdf"]:+VMD}}

		printf '%-8s %-15s %-6s %-6s %-7s %-16s %-10s %s\n' \
			"$desc" "$bdf" "${pci_ids_vendor["$bdf"]#0x}" "${pci_ids_device["$bdf"]#0x}" \
			"$node" "${driver:--}" "${name:-}" "${blknames[*]:--}"
	done
}

function status_freebsd() {
	local pci

	status_print() (
		local type=$1
		local dev driver

		shift

		for pci; do
			printf '%-8s %-15s %-6s %-6s %-16s\n' \
				"$type" \
				"$pci" \
				"${pci_ids_vendor["$pci"]}" \
				"${pci_ids_device["$pci"]}" \
				"${pci_bus_driver["$pci"]}"
		done | sort -k2,2
	)

	local contigmem=present
	local contigmem_buffer_size
	local contigmem_num_buffers

	if ! kldstat -q -m contigmem; then
		contigmem="not present"
	fi
	if ! contigmem_buffer_size=$(kenv hw.contigmem.buffer_size 2> /dev/null); then
		contigmem_buffer_size="not set"
	fi
	if ! contigmem_num_buffers=$(kenv hw.contigmem.num_buffers 2> /dev/null); then
		contigmem_num_buffers="not set"
	fi

	cat <<- BSD_INFO
		Contigmem ($contigmem)
		Buffer Size: $contigmem_buffer_size
		Num Buffers: $contigmem_num_buffers

	BSD_INFO

	printf '\n%-8s %-15s %-6s %-6s %-16s\n' \
		"Type" "BDF" "Vendor" "Device" "Driver" >&2

	status_print "NVMe" "${!nvme_d[@]}"
	status_print "I/OAT" "${!ioat_d[@]}"
	status_print "DSA" "${!dsa_d[@]}"
	status_print "IAA" "${!iaa_d[@]}"
	status_print "VMD" "${!vmd_d[@]}"
}

function configure_freebsd_pci() {
	local BDFS

	BDFS+=("${!nvme_d[@]}")
	BDFS+=("${!ioat_d[@]}")
	BDFS+=("${!dsa_d[@]}")
	BDFS+=("${!iaa_d[@]}")
	BDFS+=("${!vmd_d[@]}")

	# Drop the domain part from all the addresses
	BDFS=("${BDFS[@]#*:}")

	local IFS=","
	kldunload nic_uio.ko || true
	kenv hw.nic_uio.bdfs="${BDFS[*]}"
	kldload nic_uio.ko
}

function configure_freebsd() {
	if ! check_for_driver_freebsd; then
		echo "DPDK drivers (contigmem and/or nic_uio) are missing, aborting" >&2
		return 1
	fi
	configure_freebsd_pci
	# If contigmem is already loaded but the HUGEMEM specified doesn't match the
	#  previous value, unload contigmem so that we can reload with the new value.
	if kldstat -q -m contigmem; then
		# contigmem may be loaded, but the kernel environment doesn't have to
		# be necessarily set at this point. If it isn't, kenv will fail to
		# pick up the hw. options. Handle it.
		if ! contigmem_num_buffers=$(kenv hw.contigmem.num_buffers); then
			contigmem_num_buffers=-1
		fi 2> /dev/null
		if ((contigmem_num_buffers != HUGEMEM / 256)); then
			kldunload contigmem.ko
		fi
	fi
	if ! kldstat -q -m contigmem; then
		kenv hw.contigmem.num_buffers=$((HUGEMEM / 256))
		kenv hw.contigmem.buffer_size=$((256 * 1024 * 1024))
		kldload contigmem.ko
	fi
}

function reset_freebsd() {
	kldunload contigmem.ko || true
	kldunload nic_uio.ko || true
}

CMD=reset cache_pci_bus

mode=$1

if [ -z "$mode" ]; then
	mode="config"
fi

: ${HUGEMEM:=2048}
: ${PCI_ALLOWED:=""}
: ${PCI_BLOCKED:=""}

if [ -n "$NVME_ALLOWED" ]; then
	PCI_ALLOWED="$PCI_ALLOWED $NVME_ALLOWED"
fi

if [ -n "$SKIP_PCI" ]; then
	PCI_ALLOWED="none"
fi

if [ -z "$TARGET_USER" ]; then
	TARGET_USER="$SUDO_USER"
	if [ -z "$TARGET_USER" ]; then
		TARGET_USER=$(logname 2> /dev/null) || true
	fi
fi

collect_devices "$mode"

if [[ $mode == reset && $PCI_BLOCK_SYNC_ON_RESET == yes ]]; then
	# Note that this will wait only for the first block device attached to
	# a given storage controller. For nvme this may miss some of the devs
	# in case multiple namespaces are being in place.
	# FIXME: Wait for nvme controller(s) to be in live state and determine
	# number of configured namespaces, build list of potential block devs
	# and pass them to sync_dev_uevents. Is it worth the effort?
	bdfs_to_wait_for=()
	for bdf in "${!all_devices_d[@]}"; do
		((all_devices_d["$bdf"] == 0)) || continue
		if [[ -n ${nvme_d["$bdf"]} || -n ${virtio_d["$bdf"]} ]]; then
			[[ $(collect_driver "$bdf") != "${drivers_d["$bdf"]}" ]] || continue
			bdfs_to_wait_for+=("$bdf")
		fi
	done
	if ((${#bdfs_to_wait_for[@]} > 0)); then
		echo "Waiting for block devices as requested"
		export UEVENT_TIMEOUT=5 DEVPATH_LOOKUP=yes DEVPATH_SUBSYSTEM=pci
		"$rootdir/scripts/sync_dev_uevents.sh" \
			block/disk \
			"${bdfs_to_wait_for[@]}" &
		sync_pid=$!
	fi
fi

if [[ $os == Linux ]]; then
	if [[ -n $HUGEPGSZ && ! -e /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}kB ]]; then
		echo "${HUGEPGSZ}kB is not supported by the running kernel, ignoring" >&2
		unset -v HUGEPGSZ
	fi

	HUGEPGSZ=${HUGEPGSZ:-$(grep Hugepagesize /proc/meminfo | cut -d : -f 2 | tr -dc '0-9')}
	HUGEPGSZ_MB=$((HUGEPGSZ / 1024))
	: ${NRHUGE=$(((HUGEMEM + HUGEPGSZ_MB - 1) / HUGEPGSZ_MB))}

	if [ "$mode" == "config" ]; then
		configure_linux
	elif [ "$mode" == "cleanup" ]; then
		cleanup_linux
		clear_hugepages
	elif [ "$mode" == "reset" ]; then
		reset_linux
	elif [ "$mode" == "status" ]; then
		status_linux
	elif [ "$mode" == "help" ]; then
		usage $0
	else
		usage $0 "Invalid argument '$mode'"
	fi
else
	if [ "$mode" == "config" ]; then
		configure_freebsd
	elif [ "$mode" == "reset" ]; then
		reset_freebsd
	elif [ "$mode" == "cleanup" ]; then
		echo "setup.sh cleanup function not yet supported on $os"
	elif [ "$mode" == "status" ]; then
		status_freebsd
	elif [ "$mode" == "help" ]; then
		usage $0
	else
		usage $0 "Invalid argument '$mode'"
	fi
fi

if [[ -e /proc/$sync_pid/status ]]; then
	wait "$sync_pid"
fi
