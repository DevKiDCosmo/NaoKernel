#!/bin/bash
set -e
echo "Installing NaoKernel dependencies..."

# Update package manager
apt-get update
apt-get upgrade -y

# Install essential build tools
apt-get install -y \
    build-essential \
    git \
    curl \
    wget \
    make \
    gcc \
    g++

apt-get install -y nasm qemu-system-i386
echo "NaoKernel installation complete!"