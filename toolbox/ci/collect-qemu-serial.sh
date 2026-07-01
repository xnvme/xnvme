#!/usr/bin/env bash
# Collect QEMU guest serial.output files into a destination directory and tail
# each to stdout. Intended for CI: when a qemu-based cijoe workflow fails to
# reach 'login:', the guest's boot log lives at ~/guests/<name>/serial.output
# on the runner and is otherwise lost when the runner is wiped.
#
# Usage: collect-qemu-serial.sh <dest_dir> [guests_dir]
set -u

dst="${1:-.}"
guests="${2:-$HOME/guests}"
tail_bytes="${SERIAL_TAIL_BYTES:-65536}"

mkdir -p "$dst"

shopt -s nullglob
files=("$guests"/*/serial.output)
shopt -u nullglob

if [ "${#files[@]}" -eq 0 ]; then
    echo "no serial.output files under $guests"
    exit 0
fi

for f in "${files[@]}"; do
    name="$(basename "$(dirname "$f")").serial.output"
    printf '===== %s (%s bytes) =====\n' "$f" "$(wc -c < "$f")"
    tail -c "$tail_bytes" "$f" || true
    cp "$f" "$dst/$name" || true
done
