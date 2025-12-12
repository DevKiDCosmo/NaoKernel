#!/bin/bash
# ============================================================================
# NaoKernel Run Script
# Launches the kernel in QEMU
# ============================================================================

set -e  # Exit on error

# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ============================================================================
# Run Functions
# ============================================================================

setup_drives_if_needed() {
    if [[ ! -d "run" ]] || [[ ! -f "run/disk.img" ]]; then
        log_warning "Drive images not found. Setting up drives..."
        bash "$SCRIPT_DIR/setup-drives.sh"
    fi
}

run_kernel() {
    log_step "Starting NaoKernel in QEMU..."
    
    # Check if kernel binary exists
    if [[ ! -f "bin/kernel" ]]; then
        log_error "Kernel binary not found at bin/kernel"
        log_error "Please build the kernel first with: ./start.sh --build"
        exit 1
    fi
    
    # Setup drives if needed
    setup_drives_if_needed
    
    log_info "Launching QEMU with kernel..."
    log_info "Command: qemu-system-i386 -kernel bin/kernel -hda run/disk.img -fda run/floppy.img"
    log_info ""
    
    qemu-system-i386 -kernel bin/kernel -hda run/disk.img -fda run/floppy.img
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Execution"
    log_info "================================================"
    log_info ""
    
    # Check if QEMU is installed
    if ! command_exists qemu-system-i386; then
        log_error "qemu-system-i386 not found"
        log_error "Please install QEMU first"
        exit 1
    fi
    
    run_kernel
}

main "$@"
