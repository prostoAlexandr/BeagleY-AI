#!/usr/bin/env bash
set -euo pipefail
sudo apt update
sudo apt install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    gdb-multiarch
aarch64-linux-gnu-gcc -dumpmachine
