#!/bin/bash
set -eux
export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get upgrade -y
apt-get dist-upgrade -y
apt-get install -y \
    build-essential \
    cloud-utils \
    curl \
    git \
    libglib2.0-dev \
    meson \
    pipx \
    python3 \
    screen

# For reduced variability
apt-get install -y \
    linux-cpupower \
    linux-perf \
    lm-sensors

# For SPDK target-app
apt-get install -y \
    python3-pyelftools

# For producing reports with rst2pdf
apt-get install -y \
    fontconfig