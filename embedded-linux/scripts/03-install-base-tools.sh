#!/usr/bin/env bash
set -euo pipefail

sudo apt update

sudo apt install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    ninja-build \
    meson \
    autoconf \
    automake \
    libtool \
    pkg-config \
    bash \
    perl \
    findutils \
    debianutils \
    git \
    git-lfs \
    python3 \
    python3-pip \
    python3-venv \
    python3-pexpect \
    python3-git \
    python3-jinja2 \
    python3-subunit \
    python3-websockets \
    python3-cryptography \
    python3-pyelftools \
    python3-jsonschema \
    wget \
    curl \
    ca-certificates \
    gnupg \
    rsync \
    tar \
    gzip \
    bzip2 \
    xz-utils \
    zstd \
    lz4 \
    unzip \
    zip \
    cpio \
    file \
    patch \
    diffutils \
    diffstat \
    gawk \
    sed \
    grep \
    bc \
    flex \
    bison \
    swig \
    chrpath \
    texinfo \
    socat \
    libacl1 \
    libcrypt-dev \
    libssl-dev \
    libelf-dev \
    libncurses-dev \
    dwarves \
    device-tree-compiler \
    openssh-client \
    iproute2 \
    iputils-ping \
    net-tools \
    ethtool \
    dnsutils \
    tcpdump \
    netcat-openbsd \
    picocom \
    minicom \
    screen \
    fdisk \
    parted \
    dosfstools \
    e2fsprogs \
    bmap-tools \
    qemu-system-arm \
    qemu-user \
    qemu-user-static \
    nfs-kernel-server \
    tftpd-hpa \
    gdb \
    strace \
    ltrace \
    binutils \
    xxd \
    bsdextrautils \
    tree \
    locales \
    lsb-release \
    libgnutls28-dev

pip install --user yamllint --break-system-packages
sudo locale-gen en_US.UTF-8 de_DE.UTF-8
locale -a | grep -i '^en_US\.utf8$' || true
