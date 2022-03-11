#!/usr/bin/env bash
#
# Invokes clang-format for C sources and header-files, using the following clang-files:
#
# * scripts/clang-format-h, for .h header files
# * scripts/clang-format-c, for .c source files
#
# Usage
# =====
#
# # Run clang-format and log format-changes to 'scripts/source-format.log'
# ./source-format
#
# # Run clang-format and apply format-changes according
# ./source-format 1
#
# NOTE: must be executed inside the ./scripts directory
set -euo pipefail
IFS=$'\n\t'

CLANG_FORMAT_BIN="${CLANG_FORMAT_BIN:=clang-format}"

main() {
  local headers=$(find ../{include,lib,examples,tests,tools} -type f -name *.h)
  local sources=$(find ../{include,lib,examples,tests,tools} -type f -name *.c | grep -v fioe.c)
  local fioe=$(find ../{include,lib,examples,tests,tools} -type f -name *.c | grep fioe.c)

  CLANG_FORMAT_H="{$(tail -n +4 clang-format-h | tr '\n' ',')}"
  CLANG_FORMAT_C="{$(tail -n +4 clang-format-c | tr '\n' ',')}"
  CLANG_FORMAT_FIOE="{$(tail -n +4 clang-format-fioe | tr '\n' ',')}"

  if [[ $# -eq 1 ]]; then
      $CLANG_FORMAT_BIN -i -style=$CLANG_FORMAT_H ${headers} && \
      $CLANG_FORMAT_BIN -i -style=$CLANG_FORMAT_C ${sources} && \
      $CLANG_FORMAT_BIN -i -style=$CLANG_FORMAT_FIOE ${fioe} && \
      return $?
  fi

  if [[ -f source-format.log ]]; then
    mv source-format.log "source-format.log.${RANDOM}"
  fi

  diff -u <(cat ${headers}) <($CLANG_FORMAT_BIN -style=$CLANG_FORMAT_H ${headers})  >> source-format.log
  diff -u <(cat ${sources}) <($CLANG_FORMAT_BIN -style=$CLANG_FORMAT_C ${sources}) >> source-format.log
  diff -u <(cat ${fioe}) <($CLANG_FORMAT_BIN -style=$CLANG_FORMAT_FIOE ${fioe}) >> source-format.log
  if grep "@@" source-format.log; then
    echo "clang-format found something, please check: scripts/source-format.log"
    return 1
  fi

  return 0
}

hash $CLANG_FORMAT_BIN 2>/dev/null || {
  echo >&2 "Please install clang-format-14 or set env. var CLANG_FORMAT_BIN."; exit 1;
}

main $@
exit $?
