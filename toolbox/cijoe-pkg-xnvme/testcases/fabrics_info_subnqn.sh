#!/usr/bin/env bash
#
# Verify that the `xnvme info` runs without error using '--subnqn' for Fabrics
#
# shellcheck disable=SC2119
#
CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
export CIJ_TEST_NAME
# shellcheck source=modules/cijoe.sh
source "$CIJ_ROOT/modules/cijoe.sh"
test.enter

: "${NVMET_IP:?Must be set and non-empty}"
: "${NVMET_PORT:?Must be set and non-empty}"
: "${NVMET_SUBNQN_PREFIX:?Must be set and non-empty}"

if ! cij.cmd "xnvme info ${NVMET_IP}:${NVMET_PORT} --dev-nsid=1 --subnqn ${NVMET_SUBNQN_PREFIX}1"; then
    test.fail
fi

if ! cij.cmd "xnvme info ${NVMET_IP}:${NVMET_PORT} --dev-nsid=1 --subnqn ${NVMET_SUBNQN_PREFIX}2"; then
    test.fail
fi

if ! cij.cmd "xnvme info ${NVMET_IP}:${NVMET_PORT} --dev-nsid=1"; then
    test.fail
fi

test.pass
