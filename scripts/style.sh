#!/usr/bin/env bash
#
# Basic code format according to astyle definition
#
set -euo pipefail
IFS=$'\n\t'

ASTYLE_FNAME=xnvme.astyle

if [[ ! -f "$ASTYLE_FNAME" ]]; then
  echo "The style script must be run from within the source 'scripts/' folder"
  exit 1
fi

astyle --options=xnvme.astyle ../include/*.h
astyle --options=xnvme.astyle ../src/*.c
astyle --options=xnvme.astyle ../examples/*.c
astyle --options=xnvme.astyle ../tests/*.c
astyle --options=xnvme.astyle ../tools/*.c