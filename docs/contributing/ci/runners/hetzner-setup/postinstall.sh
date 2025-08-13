#!/bin/bash
set -eux

for task in ./tasks/*.sh; do
    echo "=== Running $(basename "$task") ==="
    $task
done
