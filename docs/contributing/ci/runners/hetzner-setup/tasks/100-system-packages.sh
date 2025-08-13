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
    qemu-kvm \
    screen

# For building and running custom qemu
apt-get install -y \
    autoconf \
    automake \
    bc \
    bison \
    bridge-utils \
    build-essential \
    ca-certificates \
    cpio \
    curl \
    file \
    flex \
    gawk \
    git \
    libaio-dev \
    libglib2.0-0 \
    libglib2.0-dev \
    libguestfs-tools \
    liblzo2-dev \
    libpixman-1-0 \
    libpixman-1-dev \
    libpmem-dev \
    libslirp-dev \
    libvirt0 \
    libvirt-dev \
    meson \
    ninja-build \
    python3 \
    python3-dev \
    python3-distutils \
    python3-setuptools \
    python3-venv \
    rsync \
    texinfo \
    uuid-dev

# For producing reports with rst2pdf
apt-get install -y \
    fontconfig