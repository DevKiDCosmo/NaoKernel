#!/bin/bash

# ============================================================================
# NaoKernel Drive Setup Script
# ============================================================================

set -e  # Exit on error
# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if run on Linux
if [[ "$(uname -s)" != "Linux" ]]; then
    log_error "This script is intended to run on Linux systems only."
    exit 1
fi

log_info "Updating package lists..."
sudo apt-get update
log_success "Package lists updated."

# If run dir is there, remove it
if [ -d "run" ]; then
    log_warning "'run' directory exists. Removing it..."
    rm -rf run
    log_success "'run' directory removed."
fi

# Disk Creation
mkdir run
cd run

dd if=/dev/zero of=disk.img bs=1M count=10
qemu-img create -f raw disk.img 10M
mkfs.fat -F 16 disk.img

# For floppy disk image
dd if=/dev/zero of=disk.img bs=512 count=2880
mkfs.fat -F 12 disk.img

cd ..