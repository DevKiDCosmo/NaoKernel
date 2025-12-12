#!/bin/bash

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Utility Functions
# ============================================================================

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

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Install package with appropriate package manager
install_package() {
    local package=$1
    
    if command_exists apt-get; then
        log_info "Installing $package with apt-get..."
        sudo apt-get update && sudo apt-get install -y "$package"
    elif command_exists yum; then
        log_info "Installing $package with yum..."
        sudo yum install -y "$package"
    elif command_exists pacman; then
        log_info "Installing $package with pacman..."
        sudo pacman -S --noconfirm "$package"
    elif command_exists brew; then
        log_info "Installing $package with brew..."
        brew install "$package"
    elif command_exists dnf; then
        log_info "Installing $package with dnf..."
        sudo dnf install -y "$package"
    elif command_exists zypper; then
        log_info "Installing $package with zypper..."
        sudo zypper install -y "$package"
    else
        log_error "No supported package manager found. Please install $package manually."
        return 1
    fi
}

# ============================================================================
# Environment Detection
# ============================================================================

OS_TYPE=$(uname)
log_info "Detected OS: $OS_TYPE"

# Detect if running in Docker
if [[ -f "/.dockerenv" ]] || grep -q docker /proc/1/cgroup 2>/dev/null; then
    ENVIRONMENT="DOCKER"
    log_info "Running inside Docker container"
elif [[ "$OS_TYPE" == "Darwin" ]]; then
    ENVIRONMENT="MACOS"
    log_info "Detected macOS"
elif [[ -f "/etc/os-release" ]]; then
    source /etc/os-release
    ENVIRONMENT="LINUX"
    log_info "Detected Linux: $NAME ($ID)"
else
    ENVIRONMENT="UNKNOWN"
    log_warning "Unable to determine OS type"
fi

# ============================================================================
# Dependency Checking & Installation
# ============================================================================

check_dependencies() {
    log_info "Checking build dependencies..."
    
    case $ENVIRONMENT in
        DOCKER)
            log_info "Running in Docker - dependencies assumed installed"
            ;;
        MACOS)
            check_macos_deps
            ;;
        LINUX)
            check_linux_deps
            ;;
        *)
            log_warning "Unknown environment - skipping dependency checks"
            ;;
    esac
}

check_macos_deps() {
    # Install Homebrew if needed
    if ! command_exists brew; then
        log_info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Check and install required tools
    local tools=("gcc" "nasm" "qemu")
    for tool in "${tools[@]}"; do
        if ! command_exists "$tool"; then
            log_warning "$tool not found, installing..."
            install_package "$tool"
        else
            log_success "$tool is installed"
        fi
    done
}

check_linux_deps() {
    local tools=("gcc" "nasm" "qemu-system-x86")
    
    # Check for apt-get (Debian/Ubuntu-based)
    if command_exists apt-get; then
        sudo apt-get update
        for tool in "${tools[@]}"; do
            if ! command_exists "$tool"; then
                log_warning "$tool not found, installing..."
                install_package "$tool"
            else
                log_success "$tool is installed"
            fi
        done
    # Check for yum/dnf (RedHat-based)
    elif command_exists yum || command_exists dnf; then
        for tool in "${tools[@]}"; do
            if ! command_exists "$tool"; then
                log_warning "$tool not found, installing..."
                install_package "$tool"
            else
                log_success "$tool is installed"
            fi
        done
    # Check for pacman (Arch-based)
    elif command_exists pacman; then
        for tool in "${tools[@]}"; do
            if ! command_exists "$tool"; then
                log_warning "$tool not found, installing..."
                install_package "$tool"
            else
                log_success "$tool is installed"
            fi
        done
    else
        log_warning "Unsupported package manager. Please install dependencies manually: ${tools[@]}"
    fi
}

# ============================================================================
# Build Process
# ============================================================================

build_project() {
    log_info "Starting build process..."
    
    if [[ ! -f "docker.sh" ]]; then
        log_error "docker.sh not found"
        return 1
    fi
    
    log_info "Building with Docker..."
    bash docker.sh
    log_success "Docker build completed"
}

# ============================================================================
# Installation
# ============================================================================

run_install() {
    case $ENVIRONMENT in
        DOCKER)
            log_info "Running inside Docker - skipping installation"
            return 0
            ;;
        MACOS)
            log_info "Running on macOS - skipping install.sh (use brew for dependencies)"
            return 0
            ;;
        LINUX)
            if [[ -f "/etc/os-release" ]]; then
                source /etc/os-release
                if [[ "$ID" == "ubuntu" || "$ID_LIKE" == *"debian"* || "$ID" == *"debian"* ]]; then
                    if [[ ! -f "install.sh" ]]; then
                        log_warning "install.sh not found, skipping installation"
                        return 0
                    fi
                    log_info "Running installation for Ubuntu/Debian system..."
                    bash install.sh
                    log_success "Installation completed"
                else
                    log_info "Non-Ubuntu/Debian Linux detected - skipping install.sh"
                    log_info "Use your package manager to install necessary files"
                    return 0
                fi
            else
                log_warning "Unable to determine Linux distribution - skipping installation"
                return 0
            fi
            ;;
        *)
            log_warning "Unknown environment - skipping installation"
            return 0
            ;;
    esac
}

# ============================================================================
# Execution
# ============================================================================

run_kernel() {
    if [[ ! -f "bin/kernel" ]]; then
        log_error "Kernel binary not found at bin/kernel"
        return 1
    fi
    
    log_info "Starting NaoKernel..."
    qemu-system-i386 -kernel bin/kernel -hda run/disk.img -fda run/floppy.img
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Universal Build System"
    log_info "================================================"
    log_info ""
    
    # Check dependencies
    check_dependencies
    
    # Build project
    build_project
    
    # Run installation
    run_install
    
    # Run kernel if --run flag is provided
    if [[ "$1" == "--run" || "$1" == "run" ]]; then
        log_info ""
        log_info "Launching kernel (--run flag detected)..."
        run_kernel
    else
        log_success ""
        log_success "Build complete! Run 'qemu-system-i386 -kernel bin/kernel' to start the kernel"
        log_success "Or run '$0 --run' to build and start automatically"
    fi
}

# Run main function
main "$@"