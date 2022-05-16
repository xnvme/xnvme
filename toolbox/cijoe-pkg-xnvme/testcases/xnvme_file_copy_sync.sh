#!/usr/bin/env bash
#
# Produce a file of 1GB then copy it in a tmpfs
#
# The testcase assumes that a tmpfs is mounted at TMPFS_MP
# mount -t tmpfs -o size=4096M tmpfs /opt/tmpfs
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${TMPFS_MP:=/opt/tmpfs}"

: "${src_fpath:=${TMPFS_MP}/input.bin}"
: "${dst_fpath:=${TMPFS_MP}/output.bin}"
: "${iosize:=4096}"

if ! cij.cmd "dd if=/dev/zero of=${dst_fpath} bs=1M count=1000"; then
  test.fail
fi
if ! cij.cmd "sync"; then
  test.fail
fi
if cij.cmd "free -m "; then
  test.fail
fi
if ! cij.cmd "df -h"; then
  test.fail
fi
if ! cij.cmd "lsblk"; then
  test.fail
fi

if ! cij.cmd "xnvme_file copy-sync ${src_fpath} ${dst_fpath} --iosize=${iosize}"; then
  test.fail
fi

if cij.cmd "free -m "; then
  test.fail
fi

test.pass
