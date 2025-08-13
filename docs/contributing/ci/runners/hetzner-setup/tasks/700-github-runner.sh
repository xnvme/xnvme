#!/bin/bash
set -eux

su - ghr -c '
    cd ~ && \
    mkdir -p ~/actions-runner && cd ~/actions-runner && \
    curl -o actions-runner-linux-x64-2.327.1.tar.gz \
        -L https://github.com/actions/runner/releases/download/v2.327.1/actions-runner-linux-x64-2.327.1.tar.gz && \
    echo "d68ac1f500b747d1271d9e52661c408d56cffd226974f68b7dc813e30b9e0575  actions-runner-linux-x64-2.327.1.tar.gz" | shasum -a 256 -c && \
    tar xzf ./actions-runner-linux-x64-2.327.1.tar.gz
'
