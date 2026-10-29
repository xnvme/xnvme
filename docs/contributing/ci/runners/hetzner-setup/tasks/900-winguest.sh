#!/bin/bash
set -eux

mkdir -p /home/ghr/system_imaging/{disk,iso}
wget \
 'https://u489897-sub1:PublicAccess1!@u489897-sub1.your-storagebox.de/iso/virtio-win.iso' \
 -O /home/ghr/system_imaging/iso/virtio-win.iso

chown -R ghr:ghr /home/ghr/system_imaging

echo "# Here are the system_imaging files - BEGIN"
find /home/ghr/system_imaging
echo "# Here are the system_imaging files - END"
