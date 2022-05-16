#!/usr/bin/env bash
#
# Verify that the CLI `xnvme format` runs without error
#
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${XNVME_URI:?Must be set and non-empty}"

if ! cij.cmd "xnvme format $XNVME_URI"; then
  test.fail
fi

test.pass
