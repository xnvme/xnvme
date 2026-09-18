#!/bin/bash
set -eux

curl -fsSL https://get.docker.com -o /tmp/get-docker.sh
sh /tmp/get-docker.sh

# get-docker.sh creates the docker group but adds no one to it; ghr needs to
# run docker directly (GHA jobs invoke it as the runner user, not via sudo)
usermod -aG docker ghr
