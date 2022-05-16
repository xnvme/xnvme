#!/usr/bin/env bash
#
# Verify that CLI `xnvme library-info` runs without error
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

if ! cij.cmd "xnvme library-info"; then
  test.fail
fi

test.pass
