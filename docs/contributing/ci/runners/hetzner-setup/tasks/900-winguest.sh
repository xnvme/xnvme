#!/bin/bash
set -eux

mkdir -p /home/ghr/system_imaging/{disk,fd}
cp /usr/share/OVMF/OVMF_VARS_4M.ms.fd /home/ghr/system_imaging/fd/win11_VARS.fd
chown -R ghr:ghr /home/ghr/system_imaging