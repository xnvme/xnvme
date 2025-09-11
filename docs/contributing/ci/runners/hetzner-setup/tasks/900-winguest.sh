#!/bin/bash
set -eux

mkdir -p /home/ghr/system_imaging/{disk,fd,iso}
wget \
 'https://u489897-sub1:PublicAccess1!@u489897-sub1.your-storagebox.de/iso/virtio-win.iso' \
 -O /home/ghr/system_imaging/iso/virtio-win.iso

cp /usr/share/OVMF/OVMF_VARS_4M.ms.fd /home/ghr/system_imaging/fd/win11_VARS.fd
chown -R ghr:ghr /home/ghr/system_imaging

echo "# Here are the system_imaging files - BEGIN"
find /home/ghr/system_imaging
echo "# Here are the system_imaging files - END"
