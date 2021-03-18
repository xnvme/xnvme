#!/usr/bin/env bash
#
# Basic code format according to astyle definition
#
set -euo pipefail
IFS=$'\n\t'

hash astyle 2>/dev/null || { echo >&2 "Please install astyle."; exit 1; }

ASTYLE_FNAME=xnvme.astyle
ASTYLE_ARGS=""
if [[ $# -eq 1 ]];
then
  ASTYLE_ARGS="--dry-run"
fi

if [[ ! -f "$ASTYLE_FNAME" ]]; then
  echo "The style script must be run from within the source 'scripts/' folder"
  exit 1
fi

FILES=$(find ../{include,src,examples,tests,tools} -type f -name *.h -o -name *.c | grep -v xnvme_fioe.c)

if [[ -f astyle.log ]]; then
  mv astyle.log "astyle.log.${RANDOM}"
fi

astyle ${ASTYLE_ARGS} --options=xnvme.astyle ${FILES} >> astyle.log

if grep -iq "^formatted" astyle.log; then
  echo "# astyle ${ASTYLE_ARGS}"
  echo "astyle found something, please check: scripts/astyle.log"
  exit 1
fi

exit 0
