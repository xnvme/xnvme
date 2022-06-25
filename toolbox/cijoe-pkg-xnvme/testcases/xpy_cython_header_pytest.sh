#!/bin/bash
 #
 # Verify that `xcy_pytest` runs without error
 #
 # When this passes the xNVMe Cython bindings loadable and functional.
 #
 # shellcheck disable=SC2119
 #
 CIJ_TEST_NAME=$(basename "${BASH_SOURCE[0]}")
 export CIJ_TEST_NAME
 # shellcheck source=modules/cijoe.sh
 source "$CIJ_ROOT/modules/cijoe.sh"
 test.enter

 : "${XNVME_URI:?Must be set and non-empty}"
 : "${XNVME_BE:?Must be set and non-empty}"
 : "${XNVME_REPO:?Must be set and non-empty}"
 : "${XNVME_DEV_NSID:?Must be set and non-empty}"

 if ! cij.cmd "XNVME_URI=${XNVME_URI} XNVME_BE=${XNVME_BE} XNVME_DEV_NSID=${XNVME_DEV_NSID} python3 -m pytest --cython-collect ${XNVME_REPO}/python/xnvme-cy-header/xnvme/cython_header/tests/ -v -s"; then
   test.fail
 fi

 test.pass
