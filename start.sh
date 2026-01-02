#!/bin/bash
# ============================================================================
# NaoKernel Universal Start Script
# Main entry point for building and running NaoKernel
# ============================================================================
set -e  # Exit on error
# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/SCRIPTS/lib.sh"

# ============================================================================
# Environment Detection
# ============================================================================

detect_environment() {
    OS_TYPE=$(detect_os)
    log_info "Detected OS: $OS_TYPE"
    
    if [[ "$OS_TYPE" == "LINUX" ]]; then
        DISTRO=$(detect_distro)
        log_info "Linux distribution: $DISTRO"
    fi
}

# ============================================================================
# Dependency Management
# ============================================================================

check_dependencies() {
    log_step "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for essential tools
    if ! command_exists gcc; then
        missing_deps+=("gcc")
    fi
    
    if ! command_exists nasm; then
        missing_deps+=("nasm")
    fi
    
    if ! command_exists ld; then
        missing_deps+=("ld")
    fi
    
    if ! command_exists qemu-system-i386; then
        missing_deps+=("qemu-system-i386")
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_warning "Missing dependencies: ${missing_deps[*]}"
        
        case $OS_TYPE in
            DOCKER)
                log_error "Running in Docker - dependencies should be installed via Dockerfile"
                exit 1
                ;;
            LINUX)
                if [[ "$DISTRO" == "ubuntu" || "$DISTRO" == "debian" || "$DISTRO" == *"debian"* ]]; then
                    log_info "Run: bash SCRIPTS/install.sh"
                else
                    log_info "Please install dependencies manually for your distribution"
                fi
                exit 1
                ;;
            MACOS)
                log_info "Run: brew install gcc nasm qemu"
                exit 1
                ;;
            *)
                log_error "Please install missing dependencies manually"
                exit 1
                ;;
        esac
    else
        log_success "All dependencies found"
    fi
}

# ============================================================================
# Build Management
# ============================================================================

build_kernel() {
    log_step "Building kernel..."
    
    case $OS_TYPE in
        DOCKER)
            log_info "Using standard build in Docker environment"
            bash "$SCRIPT_DIR/SCRIPTS/build.sh"
            ;;
        LINUX)
            if [[ "$DISTRO" == "ubuntu" || "$DISTRO" == "debian" || "$DISTRO" == *"debian"* ]]; then
                log_info "Building on Ubuntu/Debian - using Docker build"
                bash "$SCRIPT_DIR/SCRIPTS/docker-build.sh"
            else
                log_info "Building natively on Linux"
                bash "$SCRIPT_DIR/SCRIPTS/build.sh"
            fi
            ;;
        MACOS)
            log_info "Building on DOCKER - macOS support via Docker"
            bash "$SCRIPT_DIR/SCRIPTS/docker-build.sh"
            #log_info "Building natively on macOS"
            #bash "$SCRIPT_DIR/SCRIPTS/build.sh"
            ;;
        *)
            log_warning "Unknown OS - attempting standard build"
            bash "$SCRIPT_DIR/SCRIPTS/build.sh"
            ;;
    esac
    
    log_success "Kernel build complete!"
}

# ============================================================================
# Execution Management
# ============================================================================

run_kernel() {
    log_step "Running kernel..."
    bash "$SCRIPT_DIR/SCRIPTS/run.sh"
}

# ============================================================================
# Status Check
# ============================================================================
status() {
    log_step "Checking project status..."

    local missing=()
    local scripts=(
        "SCRIPTS/build.sh"
        "SCRIPTS/docker-build.sh"
        "SCRIPTS/install.sh"
        "SCRIPTS/run.sh"
        "SCRIPTS/setup-drives.sh"
        "SCRIPTS/lib.sh"
    )

    for s in "${scripts[@]}"; do
        if [[ ! -f "$SCRIPT_DIR/$s" ]]; then
            missing+=("$s")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_warning "Missing helper scripts: ${missing[*]}"
    else
        log_success "All helper scripts present"
    fi

    # Build artifacts
    if [[ -f "bin/kernel" ]]; then
        log_success "Build artifact: bin/kernel present"
    else
        log_warning "bin/kernel not found"
    fi

    shopt -s nullglob
    local objs=(bin/*.o)
    if [[ ${#objs[@]} -gt 0 ]]; then
        log_info "Object files in bin/: ${#objs[@]}"
    else
        log_warning "No object files found in bin/"
    fi
    shopt -u nullglob

    # Check for downloaded/run assets
    if [[ -d "run" ]]; then
        if [[ -f "run/disk.dir" ]]; then
            log_success "Drive images present (run/disk.dir)"
        else
            log_warning "run/ exists but run/disk.dir missing"
        fi
    else
        log_warning "No run/ directory found (drive images may be missing)"
    fi

    # Check common external tool availability
    if command_exists qemu-system-i386; then
        log_success "qemu-system-i386 available"
    else
        log_info "qemu-system-i386 not found (may need installation)"
    fi

    log_success "Status check complete"
}

# ============================================================================
# Help Text
# ============================================================================

show_help() {
    cat << EOF
NaoKernel Universal Start Script

Usage: $0 [OPTIONS]

OPTIONS:
    --help, -h          Show this help message
    --build, -b         Build the kernel
    --run, -r           Run the kernel (builds first if needed)
    --runtemp, -t       Run the kernel with temporary drives
    --clean, -c         Clean build artifacts
    --status, -s        Show project status (scripts, artifacts, downloads)
    --install, -i       Install dependencies (Ubuntu/Debian only)
    --setup-drives      Setup disk and floppy images (Ubuntu only)
    --docker-build, -d  Force Docker build (Ubuntu compilation)
    --native-build, -n  Force native build (direct compilation)

    --force-build, -f   Force rebuild even if kernel exists (with run and runtemp)

EXAMPLES:
    $0 --build          Build the kernel
    $0 --run            Build and run the kernel
    $0 --install        Install dependencies on Ubuntu/Debian
    
SCRIPTS LOCATION:
    All helper scripts are in: SCRIPTS/
    - SCRIPTS/build.sh          Native build
    - SCRIPTS/docker-build.sh   Docker-based build (Ubuntu)
    - SCRIPTS/install.sh        Install deps (Ubuntu/Debian)
    - SCRIPTS/run.sh            Run kernel in QEMU
    - SCRIPTS/setup-drives.sh   Setup disk images
    - SCRIPTS/lib.sh            Shared utilities and logging

EOF
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Universal Build & Run System"
    log_info "================================================"
    log_info ""
    
    # Parse arguments
    local action="build_and_run"
    local force_build=false
    
    # Check for force build flag in all arguments
    for arg in "$@"; do
        if [[ "$arg" == "-f" || "$arg" == "--force-build" ]]; then
            force_build=true
            break
        fi
    done
    
    case "${1:-}" in
        --help|-h)
            show_help
            exit 0
            ;;
        --build|-b)
            action="build"
            ;;
        --run|-r)
            action="run"
            ;;
        --runtemp|-t)
            action="run_temp"
            ;;
        --clean|-c)
            action="clean"
            ;;
        --install|-i)
            action="install"
            ;;
        --setup-drives)
            action="setup_drives"
            ;;
        --status|-s)
            action="status"
            ;;
        --docker-build|-d)
            action="docker_build"
            ;;
        --native-build|-n)
            action="native_build"
            ;;
        "")
            action="build_and_run"
            ;;
        *)
            log_error "Unknown option: $1"
            log_info "Use --help for usage information"
            exit 1
            ;;
    esac
    
    # Detect environment
    detect_environment
    
    # Execute action
    case $action in
        build)
            check_dependencies
            build_kernel
            ;;
        run)
            if [[ ! -f "bin/kernel" ]] || [[ "$force_build" == true ]]; then
                if [[ "$force_build" == true ]]; then
                    log_info "Force rebuild requested..."
                else
                    log_warning "Kernel not built yet. Building first..."
                fi
                check_dependencies
                build_kernel
            fi
            run_kernel
            ;;
        run_temp)
            if [[ ! -f "bin/kernel" ]] || [[ "$force_build" == true ]]; then
                if [[ "$force_build" == true ]]; then
                    log_info "Force rebuild requested..."
                else
                    log_warning "Kernel not built yet. Building first..."
                fi
                check_dependencies
                build_kernel
            fi
            log_step "Setting up temporary drives..."
            run_kernel
            rm -rf run
            log_success "Temporary drives removed"
            ;;
        build_and_run)
            check_dependencies
            build_kernel
            run_kernel
            ;;
        clean)
            log_step "Cleaning build artifacts..."
            rm -rf bin/*.o bin/kernel run/
            log_success "Clean complete"
            ;;
        install)
            if [[ "$OS_TYPE" == "LINUX" ]]; then
                bash "$SCRIPT_DIR/SCRIPTS/install.sh"
            else
                log_error "Installation script is only for Ubuntu/Debian Linux"
                log_info "Please install dependencies manually for your system"
                exit 1
            fi
            ;;
        setup_drives)
            bash "$SCRIPT_DIR/SCRIPTS/setup-drives.sh"
            ;;
        docker_build)
            bash "$SCRIPT_DIR/SCRIPTS/docker-build.sh"
            ;;
        native_build)
            log_step "Building kernel natively..."
            check_dependencies
            bash "$SCRIPT_DIR/SCRIPTS/build.sh"
            log_success "Native build complete!"
            ;;
        status)
            status
            ;;
    esac
    
    log_success "Operation complete!"
}

main "$@"
