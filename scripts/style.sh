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

FILES=$(find ../{include,src,examples,tests,tools} -type f -name *.h -o -name *.c)

astyle --options=xnvme.astyle ${FILES}
