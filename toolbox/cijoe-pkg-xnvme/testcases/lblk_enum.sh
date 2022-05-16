#!/usr/bin/env bash
#
# Verify that the CLI `lblk enum` runs without error
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

if ! cij.cmd "lblk enum"; then
  test.fail
fi

test.pass
