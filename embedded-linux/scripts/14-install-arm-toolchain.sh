#!/usr/bin/env bash
set -euo pipefail
sudo apt update
sudo apt install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf \
    libc6-dev-armhf-cross \
    gdb-multiarch
arm-linux-gnueabihf-gcc -dumpmachine
