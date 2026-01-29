
#!/usr/bin/env bash
#
# These are the tools needed for generating the docs
#

export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical
apt-get -qy update
apt-get -qy \
        -o "Dpkg::Options::=--force-confdef" \
        -o "Dpkg::Options::=--force-confold" upgrade
apt-get -qy autoclean
apt-get -qy install \
        bash \
        doxygen \
        git \
        graphviz \
        pipx \
        pkg-config \
        python3 \
        python3-pip \
        universal-ctags
