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

hash astyle 2>/dev/null || { echo >&2 "Please install astyle."; exit 1; }

FILES=$(find ../{include,src,examples,tests,tools} -type f -name *.h -o -name *.c | grep -v xnvme_fioe.c)

astyle --options=xnvme.astyle ${FILES} >> astyle.log
if grep -iq "^formatted" astyle.log; then
  echo "astyle did changes, please check"
  exit 1
fi

exit 0
