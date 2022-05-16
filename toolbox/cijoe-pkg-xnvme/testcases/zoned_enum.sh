#!/usr/bin/env bash
#
# Verify that CLI `zoned enum` runs without error
#
# Fundamental check
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${XNVME_URI:?Must be set and non-empty}"

if ! cij.cmd "zoned enum"; then
  test.fail
fi

test.pass
