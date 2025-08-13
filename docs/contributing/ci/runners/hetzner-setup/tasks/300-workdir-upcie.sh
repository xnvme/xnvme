#!/bin/bash
set -eux

mkdir -p /root/{git,workdir}

git clone https://github.com/safl/upcie.git /root/git/upcie
cd /root/git/upcie
make clean config build install
