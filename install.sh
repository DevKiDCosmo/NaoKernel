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

apt-get install -y nasm qemu

# Add project-specific dependencies here
# apt-get install -y <package-name>

echo "NaoKernel installation complete!"