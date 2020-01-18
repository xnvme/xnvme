#!/usr/bin/env bash
#
# This script creates a source-archive of the given git-version-tag on which to
# base .rpm and .debs on. It uses 'git-archive-all' to do the heavy sub-module
# stuff.
#
# Use naming conventions compatible with expectations of Debian upstream source
# archives
#
PROJECT=xnvme
VERSION=$(python scripts/xnvme_ver.py --cml CMakeLists.txt)

REF="v${VERSION}"

PREFIX="${PROJECT}-${VERSION}"
BUILD="build"
DEST="${BUILD}/${PREFIX}.src.tar.gz"

# Check that git-archive-all is installed
if ! git-archive-all --help &> /dev/null; then
  echo "# This script uses 'git-archive-all', please install:"
  echo "# pip install git-archive-all"
  exit 1
fi

# Check that generator is run from xNVMe repos-root
DLIST="tools cmake docs examples include scripts src tests third-party"
for DNAME in $DLIST; do
  if [[ ! -d "$DNAME" ]]; then
    echo "# FAILED: could not find dir($DNAME)"
    echo "# INFO: script('$0'); must be executed from repository root"
    exit 1
  fi
done

mkdir "${BUILD}"

# Now create the archive!
git-archive-all --prefix="${PREFIX}" --force-submodules $DEST

echo "# $0: DONE!"
ls -lh $DEST
