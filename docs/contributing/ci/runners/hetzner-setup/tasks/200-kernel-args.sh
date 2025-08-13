#!/bin/bash
set -eux

grep -q "nosmt" /etc/default/grub || \
    sed -i '/^GRUB_CMDLINE_LINUX=/ s/"$/ nosmt"/' /etc/default/grub
update-grub2
