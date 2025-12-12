#!/bin/bash
# ============================================================================
# NaoKernel Installation Script
# For Ubuntu/Debian-based systems only
# ============================================================================

set -e  # Exit on error

# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ============================================================================
# Installation Functions
# ============================================================================

check_ubuntu() {
    local distro=$(detect_distro)
    
    if [[ "$distro" != "ubuntu" && "$distro" != "debian" && "$distro" != *"debian"* ]]; then
        log_error "This script is for Ubuntu/Debian-based systems only"
        log_error "Detected distribution: $distro"
        log_error "Please install dependencies manually for your system"
        exit 1
    fi
    
    log_info "Detected Ubuntu/Debian-based system: $distro"
}

install_dependencies() {
    log_step "Installing NaoKernel dependencies..."
    
    # Update package manager
    log_info "Updating package lists..."
    sudo apt-get update
    log_success "Package lists updated"
    
    log_info "Upgrading existing packages..."
    sudo apt-get upgrade -y
    log_success "Packages upgraded"
    
    # Install essential build tools
    log_info "Installing build essentials..."
    sudo apt-get install -y \
        build-essential \
        git \
        curl \
        wget \
        make \
        gcc \
        g++ \
        python3 \
        python3-pip
    log_success "Build tools installed"
    
    # Install kernel-specific tools
    log_info "Installing kernel build tools..."
    sudo apt-get install -y nasm qemu-system-i386 qemu-system-x86
    log_success "Kernel build tools installed"
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Installation (Ubuntu/Debian)"
    log_info "================================================"
    log_info ""
    
    check_ubuntu
    install_dependencies
    
    log_success ""
    log_success "NaoKernel installation complete!"
}

main "$@"
