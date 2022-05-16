#!/usr/bin/env bash
#
# Verify that CLI `xnvme sanitize` runs without error
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${XNVME_URI:?Must be set and non-empty}"

if ! cij.cmd "xnvme sanitize $XNVME_URI"; then
  test.fail
fi

test.pass
