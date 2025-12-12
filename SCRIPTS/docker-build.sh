#!/bin/bash
# ============================================================================
# NaoKernel Docker Build Script
# For building kernel using Docker container (Ubuntu environment)
# ============================================================================

set -e  # Exit on error

# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ============================================================================
# Docker Build Functions
# ============================================================================

build_docker_image() {
    log_step "Building Docker image..."
    docker build -t naokernel .
    log_success "Docker image built"
}

build_in_container() {
    log_step "Building kernel in container..."
    docker run --rm -v "$(pwd)/bin:/naokernel/bin" naokernel bash -c "bash SCRIPTS/build.sh"
    log_success "Kernel built in container"
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Docker Build System"
    log_info "================================================"
    log_info ""
    
    # Check if Docker is available
    if ! command_exists docker; then
        log_error "Docker is not installed or not in PATH"
        log_error "Please install Docker to use this build method"
        exit 1
    fi
    
    build_docker_image
    build_in_container
    
    log_success "Docker build complete!"
}

main "$@"
