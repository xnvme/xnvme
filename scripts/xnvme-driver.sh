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

/**
 * PCI class code for NVMe devices.
 *
 * Base class code 01h: mass storage
 * Subclass code 08h: non-volatile memory
 * Programming interface 02h: NVM Express
 */
#define SPDK_PCI_CLASS_NVME		0x010802

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
#define PCI_DEVICE_ID_VIRTIO_BLK_MODERN	0x1001
#define PCI_DEVICE_ID_VIRTIO_SCSI_MODERN 0x1004
#define PCI_DEVICE_ID_VIRTIO_VHOST_USER 0x1017
#define PCI_DEVICE_ID_INTEL_VMD		0x201d
"

# Common shell utility functions

# Check if PCI device is on PCI_WHITELIST and not on PCI_BLACKLIST
# Env:
# if PCI_WHITELIST is empty assume device is whitelistened
# if PCI_BLACKLIST is empty assume device is NOT blacklistened
# Params:
# $1 - PCI BDF
function pci_can_use() {
	local i

	# The '\ ' part is important
	if [[ " $PCI_BLACKLIST " =~ \ $1\  ]] ; then
		return 1
	fi

	if [[ -z "$PCI_WHITELIST" ]]; then
		#no whitelist specified, bind all devices
		return 0
	fi

	for i in $PCI_WHITELIST; do
		if [ "$i" == "$1" ] ; then
			return 0
		fi
	done

	return 1
}

# This function will ignore PCI PCI_WHITELIST and PCI_BLACKLIST
function iter_all_pci_class_code() {
	local class="$(printf %02x $((0x$1)))"
	local subclass="$(printf %02x $((0x$2)))"
	local progif="$(printf %02x $((0x$3)))"

	if hash lspci &>/dev/null; then
		if [ "$progif" != "00" ]; then
			lspci -mm -n -D | \
				grep -i -- "-p${progif}" | \
				awk -v cc="\"${class}${subclass}\"" -F " " \
				'{if (cc ~ $2) print $1}' | tr -d '"'
		else
			lspci -mm -n -D | \
				awk -v cc="\"${class}${subclass}\"" -F " " \
				'{if (cc ~ $2) print $1}' | tr -d '"'
		fi
	elif hash pciconf &>/dev/null; then
		local addr=($(pciconf -l | grep -i "class=0x${class}${subclass}${progif}" | \
			cut -d$'\t' -f1 | sed -e 's/^[a-zA-Z0-9_]*@pci//g' | tr ':' ' '))
		printf "%04x:%02x:%02x:%x\n" ${addr[0]} ${addr[1]} ${addr[2]} ${addr[3]}
	else
		echo "Missing PCI enumeration utility"
		exit 1
	fi
}

# This function will ignore PCI PCI_WHITELIST and PCI_BLACKLIST
function iter_all_pci_dev_id() {
	local ven_id="$(printf %04x $((0x$1)))"
	local dev_id="$(printf %04x $((0x$2)))"

	if hash lspci &>/dev/null; then
		lspci -mm -n -D | awk -v ven="\"$ven_id\"" -v dev="\"${dev_id}\"" -F " " \
			'{if (ven ~ $3 && dev ~ $4) print $1}' | tr -d '"'
	elif hash pciconf &>/dev/null; then
		local addr=($(pciconf -l | grep -i "chip=0x${dev_id}${ven_id}" | \
			cut -d$'\t' -f1 | sed -e 's/^[a-zA-Z0-9_]*@pci//g' | tr ':' ' '))
		printf "%04x:%02x:%02x:%x\n" ${addr[0]} ${addr[1]} ${addr[2]} ${addr[3]}
	else
		echo "Missing PCI enumeration utility"
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

# This function will filter out PCI devices using PCI_WHITELIST and PCI_BLACKLIST
# See function pci_can_use()
function iter_pci_class_code() {
	local bdf=""

	for bdf in $(iter_all_pci_class_code "$@"); do
		if pci_can_use "$bdf"; then
			echo "$bdf"
		fi
	done
}

function usage()
{
	if [ $(uname) = Linux ]; then
		options="[config|reset|status|cleanup|help]"
	else
		options="[config|reset|help]"
	fi

	[[ ! -z $2 ]] && ( echo "$2"; echo ""; )
	echo "Helper script for allocating hugepages and binding NVMe, I/OAT, VMD and Virtio devices"
	echo "to a generic VFIO kernel driver. If VFIO is not available on the system, this script"
	echo "will fall back to UIO. NVMe and Virtio devices with active mountpoints will be ignored."
	echo "All hugepage operations use default hugepage size on the system (hugepagesz)."
	echo "Usage: $(basename $1) $options"
	echo
	echo "$options - as following:"
	echo "config            Default mode. Allocate hugepages and bind PCI devices."
	if [ $(uname) = Linux ]; then
		echo "cleanup            Remove any orphaned files that can be left in the system after SPDK application exit"
	fi
	echo "reset             Rebind PCI devices back to their original drivers."
	echo "                  Also cleanup any leftover spdk files/resources."
	echo "                  Hugepage memory size will remain unchanged."
	if [ $(uname) = Linux ]; then
		echo "status            Print status of all SPDK-compatible devices on the system."
	fi
	echo "help              Print this help message."
	echo
	echo "The following environment variables can be specified."
	echo "HUGEMEM           Size of hugepage memory to allocate (in MB). 2048 by default."
	echo "                  For NUMA systems, the hugepages will be evenly distributed"
	echo "                  between CPU nodes"
	echo "NRHUGE            Number of hugepages to allocate. This variable overwrites HUGEMEM."
	echo "HUGENODE          Specific NUMA node to allocate hugepages on. To allocate"
	echo "                  hugepages on multiple nodes run this script multiple times -"
	echo "                  once for each node."
	echo "PCI_WHITELIST"
	echo "PCI_BLACKLIST     Whitespace separated list of PCI devices (NVMe, I/OAT, VMD, Virtio)."
	echo "                  Each device must be specified as a full PCI address."
	echo "                  E.g. PCI_WHITELIST=\"0000:01:00.0 0000:02:00.0\""
	echo "                  To blacklist all PCI devices use a non-valid address."
	echo "                  E.g. PCI_WHITELIST=\"none\""
	echo "                  If PCI_WHITELIST and PCI_BLACKLIST are empty or unset, all PCI devices"
	echo "                  will be bound."
	echo "                  Each device in PCI_BLACKLIST will be ignored (driver won't be changed)."
	echo "                  PCI_BLACKLIST has precedence over PCI_WHITELIST."
	echo "TARGET_USER       User that will own hugepage mountpoint directory and vfio groups."
	echo "                  By default the current user will be used."
	echo "DRIVER_OVERRIDE   Disable automatic vfio-pci/uio_pci_generic selection and forcefully"
	echo "                  bind devices to the given driver."
	echo "                  E.g. DRIVER_OVERRIDE=uio_pci_generic or DRIVER_OVERRIDE=/home/public/dpdk/build/kmod/igb_uio.ko"
	exit 0
}

# In monolithic kernels the lsmod won't work. So
# back that with a /sys/modules. We also check
# /sys/bus/pci/drivers/ as neither lsmod nor /sys/modules might
# contain needed info (like in Fedora-like OS).
function check_for_driver {
	if lsmod | grep -q ${1//-/_}; then
		return 1
	fi

	if [[ -d /sys/module/${1} || \
			-d /sys/module/${1//-/_} || \
			-d /sys/bus/pci/drivers/${1} || \
			-d /sys/bus/pci/drivers/${1//-/_} ]]; then
		return 2
	fi
	return 0
}

function pci_dev_echo() {
	local bdf="$1"
	local vendor="$(cat /sys/bus/pci/devices/$bdf/vendor)"
	local device="$(cat /sys/bus/pci/devices/$bdf/device)"
	shift
	echo "$bdf (${vendor#0x} ${device#0x}): $@"
}

function linux_bind_driver() {
	bdf="$1"
	driver_name="$2"
	old_driver_name="no driver"
	ven_dev_id=$(lspci -n -s $bdf | cut -d' ' -f3 | sed 's/:/ /')

	if [ -e "/sys/bus/pci/devices/$bdf/driver" ]; then
		old_driver_name=$(basename $(readlink /sys/bus/pci/devices/$bdf/driver))

		if [ "$driver_name" = "$old_driver_name" ]; then
			pci_dev_echo "$bdf" "Already using the $old_driver_name driver"
			return 0
		fi

		echo "$ven_dev_id" > "/sys/bus/pci/devices/$bdf/driver/remove_id" 2> /dev/null || true
		echo "$bdf" > "/sys/bus/pci/devices/$bdf/driver/unbind"
	fi

	pci_dev_echo "$bdf" "$old_driver_name -> $driver_name"

	echo "$ven_dev_id" > "/sys/bus/pci/drivers/$driver_name/new_id" 2> /dev/null || true
	echo "$bdf" > "/sys/bus/pci/drivers/$driver_name/bind" 2> /dev/null || true

	iommu_group=$(basename $(readlink -f /sys/bus/pci/devices/$bdf/iommu_group))
	if [ -e "/dev/vfio/$iommu_group" ]; then
		if [ -n "$TARGET_USER" ]; then
			chown "$TARGET_USER" "/dev/vfio/$iommu_group"
		fi
	fi
}

function linux_unbind_driver() {
	local bdf="$1"
	local ven_dev_id=$(lspci -n -s $bdf | cut -d' ' -f3 | sed 's/:/ /')
	local old_driver_name="no driver"

	if [ -e "/sys/bus/pci/devices/$bdf/driver" ]; then
		old_driver_name=$(basename $(readlink /sys/bus/pci/devices/$bdf/driver))
		echo "$ven_dev_id" > "/sys/bus/pci/devices/$bdf/driver/remove_id" 2> /dev/null || true
		echo "$bdf" > "/sys/bus/pci/devices/$bdf/driver/unbind"
	fi

	pci_dev_echo "$bdf" "$old_driver_name -> no driver"
}

function linux_hugetlbfs_mounts() {
	mount | grep ' type hugetlbfs ' | awk '{ print $3 }'
}

function get_nvme_name_from_bdf {
	local blknames=()

	set +e
	nvme_devs=$(lsblk -d --output NAME | grep "^nvme")
	set -e
	for dev in $nvme_devs; do
		link_name=$(readlink /sys/block/$dev/device/device) || true
		if [ -z "$link_name" ]; then
			link_name=$(readlink /sys/block/$dev/device)
		fi
		link_bdf=$(basename "$link_name")
		if [ "$link_bdf" = "$1" ]; then
			blknames+=($dev)
		fi
	done

	printf '%s\n' "${blknames[@]}"
}

function get_virtio_names_from_bdf {
	blk_devs=$(lsblk --nodeps --output NAME)
	virtio_names=''

	for dev in $blk_devs; do
		if readlink "/sys/block/$dev" | grep -q "$1"; then
			virtio_names="$virtio_names $dev"
		fi
	done

	eval "$2='$virtio_names'"
}

function configure_linux_pci {
	local driver_path=""
	driver_name=""
	if [[ -n "${DRIVER_OVERRIDE}" ]]; then
		driver_path="${DRIVER_OVERRIDE%/*}"
		driver_name="${DRIVER_OVERRIDE##*/}"
		# path = name -> there is no path
		if [[ "$driver_path" = "$driver_name" ]]; then
			driver_path=""
		fi
	elif [[ -n "$(ls /sys/kernel/iommu_groups)" || \
	     (-e /sys/module/vfio/parameters/enable_unsafe_noiommu_mode && \
	     "$(cat /sys/module/vfio/parameters/enable_unsafe_noiommu_mode)" == "Y") ]]; then
		driver_name=vfio-pci
	elif modinfo uio_pci_generic >/dev/null 2>&1; then
		driver_name=uio_pci_generic
	else
		echo "No valid drivers found [vfio-pci, uio_pci_generic, igb_uio]. Please either enable the vfio-pci or uio_pci_generic"
		echo "kernel modules, or have SPDK build the igb_uio driver by running ./configure --with-igb-uio-driver and recompiling."
		return 1
	fi

	# modprobe assumes the directory of the module. If the user passes in a path, we should use insmod
	if [[ -n "$driver_path" ]]; then
		insmod $driver_path || true
	else
		modprobe $driver_name
	fi

	# NVMe
	for bdf in $(iter_all_pci_class_code 01 08 02); do
		blknames=()
		if ! pci_can_use $bdf; then
			pci_dev_echo "$bdf" "Skipping un-whitelisted NVMe controller at $bdf"
			continue
		fi

		mount=false
		for blkname in $(get_nvme_name_from_bdf $bdf); do
			mountpoints=$(lsblk /dev/$blkname --output MOUNTPOINT -n | wc -w)
			if [ "$mountpoints" != "0" ]; then
				mount=true
				blknames+=($blkname)
			fi
		done

		if ! $mount; then
			linux_bind_driver "$bdf" "$driver_name"
		else
			for name in ${blknames[@]}; do
				pci_dev_echo "$bdf" "Active mountpoints on /dev/$name, so not binding PCI dev"
			done
		fi
	done

	# IOAT
	TMP=$(mktemp)
	#collect all the device_id info of ioat devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_IOAT" | awk -F"x" '{print $2}' > $TMP

	for dev_id in $(cat $TMP); do
		for bdf in $(iter_all_pci_dev_id 8086 $dev_id); do
			if ! pci_can_use $bdf; then
				pci_dev_echo "$bdf" "Skipping un-whitelisted I/OAT device"
				continue
			fi

			linux_bind_driver "$bdf" "$driver_name"
		done
	done
	rm $TMP

	# virtio
	TMP=$(mktemp)
	#collect all the device_id info of virtio devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_VIRTIO" | awk -F"x" '{print $2}' > $TMP

	for dev_id in $(cat $TMP); do
		for bdf in $(iter_all_pci_dev_id 1af4 $dev_id); do
			if ! pci_can_use $bdf; then
				pci_dev_echo "$bdf" "Skipping un-whitelisted Virtio device at $bdf"
				continue
			fi
			blknames=''
			get_virtio_names_from_bdf "$bdf" blknames
			for blkname in $blknames; do
				if [ "$(lsblk /dev/$blkname --output MOUNTPOINT -n | wc -w)" != "0" ]; then
					pci_dev_echo "$bdf" "Active mountpoints on /dev/$blkname, so not binding"
					continue 2
				fi
			done

			linux_bind_driver "$bdf" "$driver_name"
		done
	done
	rm $TMP

	# VMD
	TMP=$(mktemp)
	#collect all the device_id info of vmd devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_VMD" | awk -F"x" '{print $2}' > $TMP

	for dev_id in $(cat $TMP); do
		for bdf in $(iter_pci_dev_id 8086 $dev_id); do
			if [[ -z "$PCI_WHITELIST" ]] || ! pci_can_use $bdf; then
				echo "Skipping un-whitelisted VMD device at $bdf"
				continue
			fi

			linux_bind_driver "$bdf" "$driver_name"
                        echo " VMD generic kdrv: " "$bdf" "$driver_name"
		done
	done
	rm $TMP

	echo "1" > "/sys/bus/pci/rescan"
}

function cleanup_linux {
	shopt -s extglob nullglob
	dirs_to_clean=""
	dirs_to_clean="$(echo {/var/run,/tmp}/dpdk/spdk{,_pid}+([0-9])) "
	if [[ -d $XDG_RUNTIME_DIR && $XDG_RUNTIME_DIR != *" "* ]]; then
		dirs_to_clean+="$(readlink -e assert_not_empty $XDG_RUNTIME_DIR/dpdk/spdk{,_pid}+([0-9]) || true) "
	fi

	files_to_clean=""
	for dir in $dirs_to_clean; do
		files_to_clean+="$(echo $dir/*) "
	done
	shopt -u extglob nullglob

	files_to_clean+="$(ls -1 /dev/shm/* | grep -E '(spdk_tgt|iscsi|vhost|nvmf|rocksdb|bdevio|bdevperf)_trace|spdk_iscsi_conns' || true) "
	files_to_clean="$(readlink -e assert_not_empty $files_to_clean || true)"
	if [[ -z "$files_to_clean" ]]; then
		echo "Clean"
		return 0;
	fi

	shopt -s extglob
	for fd_dir in $(echo /proc/+([0-9])); do
		opened_files+="$(readlink -e assert_not_empty $fd_dir/fd/* || true)"
	done
	shopt -u extglob

	if [[ -z "$opened_files" ]]; then
		echo "Can't get list of opened files!"
		exit 1
	fi

	echo 'Cleaning'
	for f in $files_to_clean; do
		if ! echo "$opened_files" | grep -E -q "^$f\$"; then
			echo "Removing:    $f"
			rm $f
		else
			echo "Still open: $f"
		fi
	done

	for dir in $dirs_to_clean; do
	if ! echo "$opened_files" | grep -E -q "^$dir\$"; then
		echo "Removing:    $dir"
		rmdir $dir
	else
		echo "Still open: $dir"
	fi
	done
	echo "Clean"

	unset dirs_to_clean files_to_clean opened_files
}

function configure_linux {
	configure_linux_pci
	hugetlbfs_mounts=$(linux_hugetlbfs_mounts)

	if [ -z "$hugetlbfs_mounts" ]; then
		hugetlbfs_mounts=/mnt/huge
		echo "Mounting hugetlbfs at $hugetlbfs_mounts"
		mkdir -p "$hugetlbfs_mounts"
		mount -t hugetlbfs nodev "$hugetlbfs_mounts"
	fi

	if [ -z "$HUGENODE" ]; then
		hugepages_target="/proc/sys/vm/nr_hugepages"
	else
		hugepages_target="/sys/devices/system/node/node${HUGENODE}/hugepages/hugepages-${HUGEPGSZ}kB/nr_hugepages"
	fi

	echo "$NRHUGE" > "$hugepages_target"
	allocated_hugepages=$(cat $hugepages_target)
	if [ "$allocated_hugepages" -lt "$NRHUGE" ]; then
		echo ""
		echo "## ERROR: requested $NRHUGE hugepages but only $allocated_hugepages could be allocated."
		echo "## Memory might be heavily fragmented. Please try flushing the system cache, or reboot the machine."
		exit 1
	fi

	if [ "$driver_name" = "vfio-pci" ]; then
		if [ -n "$TARGET_USER" ]; then
			for mount in $hugetlbfs_mounts; do
				chown "$TARGET_USER" "$mount"
				chmod g+w "$mount"
			done
		fi

		MEMLOCK_AMNT=$(ulimit -l)
		if [ "$MEMLOCK_AMNT" != "unlimited" ] ; then
			MEMLOCK_MB=$(( $MEMLOCK_AMNT / 1024 ))
			echo ""
			echo "Current user memlock limit: ${MEMLOCK_MB} MB"
			echo ""
			echo "This is the maximum amount of memory you will be"
			echo "able to use with DPDK and VFIO if run as current user."
			echo -n "To change this, please adjust limits.conf memlock "
			echo "limit for current user."

			if [ $MEMLOCK_AMNT -lt 65536 ] ; then
				echo ""
				echo "## WARNING: memlock limit is less than 64MB"
				echo -n "## DPDK with VFIO may not be able to initialize "
				echo "if run as current user."
			fi
		fi
	fi
}

function reset_linux_pci {
	# NVMe
	set +e
	check_for_driver nvme
	driver_loaded=$?
	set -e
	for bdf in $(iter_all_pci_class_code 01 08 02); do
		if ! pci_can_use $bdf; then
			pci_dev_echo "$bdf" "Skipping un-whitelisted NVMe controller $blkname"
			continue
		fi
		if [ $driver_loaded -ne 0 ]; then
			linux_bind_driver "$bdf" nvme
		else
			linux_unbind_driver "$bdf"
		fi
	done

	# IOAT
	TMP=$(mktemp)
	#collect all the device_id info of ioat devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_IOAT" | awk -F"x" '{print $2}' > $TMP

	set +e
	check_for_driver ioatdma
	driver_loaded=$?
	set -e
	for dev_id in $(cat $TMP); do
		for bdf in $(iter_all_pci_dev_id 8086 $dev_id); do
			if ! pci_can_use $bdf; then
				pci_dev_echo "$bdf" "Skipping un-whitelisted I/OAT device"
				continue
			fi
			if [ $driver_loaded -ne 0 ]; then
				linux_bind_driver "$bdf" ioatdma
			else
				linux_unbind_driver "$bdf"
			fi
		done
	done
	rm $TMP

	# virtio
	TMP=$(mktemp)
	#collect all the device_id info of virtio devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_VIRTIO" | awk -F"x" '{print $2}' > $TMP

	# TODO: check if virtio-pci is loaded first and just unbind if it is not loaded
	# Requires some more investigation - for example, some kernels do not seem to have
	#  virtio-pci but just virtio_scsi instead.  Also need to make sure we get the
	#  underscore vs. dash right in the virtio_scsi name.
	modprobe virtio-pci || true
	for dev_id in $(cat $TMP); do
		for bdf in $(iter_all_pci_dev_id 1af4 $dev_id); do
			if ! pci_can_use $bdf; then
				pci_dev_echo "$bdf" "Skipping un-whitelisted Virtio device at"
				continue
			fi
			linux_bind_driver "$bdf" virtio-pci
		done
	done
	rm $TMP

	# VMD
	TMP=$(mktemp)
	#collect all the device_id info of vmd devices.
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_VMD" | awk -F"x" '{print $2}' > $TMP

	set +e
	check_for_driver vmd
	driver_loaded=$?
	set -e
	for dev_id in $(cat $TMP); do
		for bdf in $(iter_pci_dev_id 8086 $dev_id); do
			if ! pci_can_use $bdf; then
				echo "Skipping un-whitelisted VMD device at $bdf"
				continue
			fi
			if [ $driver_loaded -ne 0 ]; then
				linux_bind_driver "$bdf" vmd
			else
				linux_unbind_driver "$bdf"
			fi
		done
	done
	rm $TMP

	echo "1" > "/sys/bus/pci/rescan"
}

function reset_linux {
	reset_linux_pci
	for mount in $(linux_hugetlbfs_mounts); do
		rm -f "$mount"/spdk*map_*
	done
	rm -f /run/.spdk*
}

function status_linux {
	echo "Hugepages"
	printf "%-6s %10s %8s / %6s\n" "node" "hugesize"  "free" "total"

	numa_nodes=0
	shopt -s nullglob
	for path in /sys/devices/system/node/node?/hugepages/hugepages-*/; do
		numa_nodes=$((numa_nodes + 1))
		free_pages=$(cat $path/free_hugepages)
		all_pages=$(cat $path/nr_hugepages)

		[[ $path =~ (node[0-9]+)/hugepages/hugepages-([0-9]+kB) ]]

		node=${BASH_REMATCH[1]}
		huge_size=${BASH_REMATCH[2]}

		printf "%-6s %10s %8s / %6s\n" $node $huge_size $free_pages $all_pages
	done
	shopt -u nullglob

	# fall back to system-wide hugepages
	if [ "$numa_nodes" = "0" ]; then
		free_pages=$(grep HugePages_Free /proc/meminfo | awk '{ print $2 }')
		all_pages=$(grep HugePages_Total /proc/meminfo | awk '{ print $2 }')
		node="-"
		huge_size="$HUGEPGSZ"

		printf "%-6s %10s %8s / %6s\n" $node $huge_size $free_pages $all_pages
	fi

	echo ""
	echo "NVMe devices"

	echo -e "BDF\t\tVendor\tDevice\tNUMA\tDriver\t\tDevice name"
	for bdf in $(iter_all_pci_class_code 01 08 02); do
		driver=$(grep DRIVER /sys/bus/pci/devices/$bdf/uevent |awk -F"=" '{print $2}')
		if [ "$numa_nodes" = "0" ]; then
			node="-"
		else
			node=$(cat /sys/bus/pci/devices/$bdf/numa_node)
		fi
		device=$(cat /sys/bus/pci/devices/$bdf/device)
		vendor=$(cat /sys/bus/pci/devices/$bdf/vendor)
		if [ "$driver" = "nvme" -a -d /sys/bus/pci/devices/$bdf/nvme ]; then
			name="\t"$(ls /sys/bus/pci/devices/$bdf/nvme);
		else
			name="-";
		fi
		echo -e "$bdf\t${vendor#0x}\t${device#0x}\t$node\t${driver:--}\t\t$name";
	done

	echo ""
	echo "I/OAT DMA"

	#collect all the device_id info of ioat devices.
	TMP=$(echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_IOAT" | awk -F"x" '{print $2}')
	echo -e "BDF\t\tVendor\tDevice\tNUMA\tDriver"
	for dev_id in $TMP; do
		for bdf in $(iter_all_pci_dev_id 8086 $dev_id); do
			driver=$(grep DRIVER /sys/bus/pci/devices/$bdf/uevent |awk -F"=" '{print $2}')
			if [ "$numa_nodes" = "0" ]; then
				node="-"
			else
				node=$(cat /sys/bus/pci/devices/$bdf/numa_node)
			fi
			device=$(cat /sys/bus/pci/devices/$bdf/device)
			vendor=$(cat /sys/bus/pci/devices/$bdf/vendor)
			echo -e "$bdf\t${vendor#0x}\t${device#0x}\t$node\t${driver:--}"
		done
	done

	echo ""
	echo "virtio"

	#collect all the device_id info of virtio devices.
	TMP=$(echo "$PIDS" | grep "PCI_DEVICE_ID_VIRTIO" | awk -F"x" '{print $2}')
	echo -e "BDF\t\tVendor\tDevice\tNUMA\tDriver\t\tDevice name"
	for dev_id in $TMP; do
		for bdf in $(iter_all_pci_dev_id 1af4 $dev_id); do
			driver=$(grep DRIVER /sys/bus/pci/devices/$bdf/uevent |awk -F"=" '{print $2}')
			if [ "$numa_nodes" = "0" ]; then
				node="-"
			else
				node=$(cat /sys/bus/pci/devices/$bdf/numa_node)
			fi
			device=$(cat /sys/bus/pci/devices/$bdf/device)
			vendor=$(cat /sys/bus/pci/devices/$bdf/vendor)
			blknames=''
			get_virtio_names_from_bdf "$bdf" blknames
			echo -e "$bdf\t${vendor#0x}\t${device#0x}\t$node\t\t${driver:--}\t\t$blknames"
		done
	done

	echo "VMD"

	#collect all the device_id info of vmd devices.
	TMP=$(echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_VMD" | awk -F"x" '{print $2}')
	echo -e "BDF\t\tNuma Node\tDriver Name"
	for dev_id in $TMP; do
		for bdf in $(iter_pci_dev_id 8086 $dev_id); do
			driver=$(grep DRIVER /sys/bus/pci/devices/$bdf/uevent |awk -F"=" '{print $2}')
			node=$(cat /sys/bus/pci/devices/$bdf/numa_node);
			echo -e "$bdf\t$node\t\t$driver"
		done
	done
}

function configure_freebsd_pci {
	TMP=$(mktemp)

	# NVMe
	GREP_STR="class=0x010802"

	# IOAT
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_IOAT" | awk -F"x" '{print $2}' > $TMP
	for dev_id in $(cat $TMP); do
		GREP_STR="${GREP_STR}\|chip=0x${dev_id}8086"
	done

	# VMD
	echo "$PIDS" | grep "PCI_DEVICE_ID_INTEL_VMD" | awk -F"x" '{print $2}' > $TMP
	for dev_id in $(cat $TMP); do
		GREP_STR="${GREP_STR}\|chip=0x${dev_id}8086"
	done

	AWK_PROG="{if (count > 0) printf \",\"; printf \"%s:%s:%s\",\$2,\$3,\$4; count++}"
	echo $AWK_PROG > $TMP

	BDFS=$(pciconf -l | grep "${GREP_STR}" | awk -F: -f $TMP)

	kldunload nic_uio.ko || true
	kenv hw.nic_uio.bdfs=$BDFS
	kldload nic_uio.ko
	rm $TMP
}

function configure_freebsd {
	configure_freebsd_pci
	# If contigmem is already loaded but the HUGEMEM specified doesn't match the
	#  previous value, unload contigmem so that we can reload with the new value.
	if kldstat -q -m contigmem; then
		if [ $(kenv hw.contigmem.num_buffers) -ne "$((HUGEMEM / 256))" ]; then
			kldunload contigmem.ko
		fi
	fi
	if ! kldstat -q -m contigmem; then
		kenv hw.contigmem.num_buffers=$((HUGEMEM / 256))
		kenv hw.contigmem.buffer_size=$((256 * 1024 * 1024))
		kldload contigmem.ko
	fi
}

function reset_freebsd {
	kldunload contigmem.ko || true
	kldunload nic_uio.ko || true
}

mode=$1

if [ -z "$mode" ]; then
	mode="config"
fi

: ${HUGEMEM:=2048}
: ${PCI_WHITELIST:=""}
: ${PCI_BLACKLIST:=""}

if [ -n "$NVME_WHITELIST" ]; then
	PCI_WHITELIST="$PCI_WHITELIST $NVME_WHITELIST"
fi

if [ -n "$SKIP_PCI" ]; then
	PCI_WHITELIST="none"
fi

if [ -z "$TARGET_USER" ]; then
	TARGET_USER="$SUDO_USER"
	if [ -z "$TARGET_USER" ]; then
		TARGET_USER=$(logname 2>/dev/null) || true
	fi
fi

if [ $(uname) = Linux ]; then
	HUGEPGSZ=$(( $(grep Hugepagesize /proc/meminfo | cut -d : -f 2 | tr -dc '0-9') ))
	HUGEPGSZ_MB=$(( $HUGEPGSZ / 1024 ))
	: ${NRHUGE=$(( (HUGEMEM + HUGEPGSZ_MB - 1) / HUGEPGSZ_MB ))}

	if [ "$mode" == "config" ]; then
		configure_linux
	elif [ "$mode" == "cleanup" ]; then
		cleanup_linux
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
		echo "nvme-driver.sh cleanup function not yet supported on $(uname)"
	elif [ "$mode" == "status" ]; then
		echo "nvme-driver.sh status function not yet supported on $(uname)"
	elif [ "$mode" == "help" ]; then
		usage $0
	else
		usage $0 "Invalid argument '$mode'"
	fi
fi
