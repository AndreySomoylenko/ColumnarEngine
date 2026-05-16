#!/usr/bin/env bash
set -euo pipefail

apt-get update

apt-get install -y --no-install-recommends \
    clang \
    libc++-dev \
    libc++abi-dev \
    cmake \
    ninja-build \
    make \
    ca-certificates

rm -rf /var/lib/apt/lists/*
