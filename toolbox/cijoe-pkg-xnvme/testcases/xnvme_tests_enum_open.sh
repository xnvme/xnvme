#!/usr/bin/env bash
#
# Verify xnvme_enumerate() and xnvme_dev_open() / xnvme_dev_close()
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${XNVME_URI:?Must be set and non-empty}"

COUNT=4

if ! cij.cmd "xnvme_tests_enum open --count ${COUNT}"; then
  test.fail
fi

test.pass
