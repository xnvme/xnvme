#!/usr/bin/env bash
# Generate a random input-file
rm -rf /tmp/data-input.bin
dd if=/dev/random of=/tmp/data-input.bin count=1 bs=512

# Use it as payload for a passthru-command (NVM Write)
xnvme pioc /dev/nvme0n1 \
        --opcode 0x1 \
        --nsid 0x1 \
        --data-input /tmp/data-input.bin \
        --data-nbytes 512

# Use it as payload for a passthru-command (NVM Read)
xnvme pioc /dev/nvme0n1 \
        --opcode 0x2 \
        --nsid 0x1 \
        --data-output /tmp/data-output.bin \
        --data-nbytes 512

# Compare
cmp /tmp/data-input.bin /tmp/data-output.bin
