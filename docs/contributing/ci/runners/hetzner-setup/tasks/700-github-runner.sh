#!/bin/bash
set -eux

su - ghr -c '
    RUNNER_VERSION="2.337.0"
    RUNNER_SHA256="70920811a4f8ad4328818682bca5c6469c1c942fab52448868071d0063816613"
    cd ~ && \
    mkdir -p ~/actions-runner && cd ~/actions-runner && \
    curl -o actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz \
        -L https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz && \
    echo "${RUNNER_SHA256}  actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz" | shasum -a 256 -c && \
    tar xzf ./actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz
'
