#!/bin/bash
set -eux

mkdir -p /home/ghr/system_imaging/{disk,fd,iso}
wget -p /home/ghr/system_imaging/iso/ https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.271-1/virtio-win.iso
cp /usr/share/OVMF/OVMF_VARS_4M.ms.fd /home/ghr/system_imaging/fd/win11_VARS.fd
chown -R ghr:ghr /home/ghr/system_imaging