#!/bin/bash
#
# Verify that `xpy_enumerate` runs without error
#
# When this passes the xNVMe Python bindings loadable and functional.
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

if ! cij.cmd "xpy_enumerate"; then
  test.fail
fi

test.pass
