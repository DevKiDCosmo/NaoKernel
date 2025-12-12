#!/bin/bash
# ============================================================================
# NaoKernel Drive Setup Script
# Creates disk and floppy images for QEMU
# ============================================================================

set -e  # Exit on error

# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ============================================================================
# Drive Setup Functions
# ============================================================================

setup_drives() {
    log_step "Setting up drive images..."
    
    # Remove existing run directory if it exists
    if [[ -d "run" ]]; then
        log_warning "'run' directory exists. Removing it..."
        rm -rf run
        log_success "'run' directory removed"
    fi
    
    # Create run directory
    log_info "Creating 'run' directory..."
    mkdir run
    cd run
    
    # Create hard disk image
    log_info "Creating hard disk image (10MB)..."
    dd if=/dev/zero of=disk.img bs=1M count=10 2>/dev/null
    qemu-img create -f raw disk.img 10M &>/dev/null
    mkfs.fat -F 16 disk.img &>/dev/null
    log_success "Hard disk image created: run/disk.img"
    
    # Create floppy disk image
    log_info "Creating floppy disk image (1.44MB)..."
    dd if=/dev/zero of=floppy.img bs=512 count=2880 2>/dev/null
    mkfs.fat -F 12 floppy.img &>/dev/null
    log_success "Floppy disk image created: run/floppy.img"
    
    cd ..
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Drive Setup"
    log_info "================================================"
    log_info ""
    
    # Check required tools
    if ! command_exists dd; then
        log_error "dd command not found"
        exit 1
    fi
    
    if ! command_exists qemu-img; then
        log_error "qemu-img command not found"
        log_error "Please install QEMU first"
        exit 1
    fi
    
    if ! command_exists mkfs.fat; then
        log_error "mkfs.fat command not found"
        log_error "Please install dosfstools first"
        exit 1
    fi
    
    setup_drives
    
    log_success ""
    log_success "Drive setup complete!"
}

main "$@"
