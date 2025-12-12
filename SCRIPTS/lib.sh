#!/bin/bash
# ============================================================================
# NaoKernel Universal Logging and Utility Library
# ============================================================================

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# ============================================================================
# Logging Functions
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

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

log_build() {
    echo -e "${MAGENTA}[BUILD]${NC} $1"
}

# ============================================================================
# Utility Functions
# ============================================================================

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Detect OS type
detect_os() {
    local os_type=$(uname)
    
    if [[ -f "/.dockerenv" ]] || grep -q docker /proc/1/cgroup 2>/dev/null; then
        echo "DOCKER"
    elif [[ "$os_type" == "Darwin" ]]; then
        echo "MACOS"
    elif [[ "$os_type" == "Linux" ]]; then
        echo "LINUX"
    else
        echo "UNKNOWN"
    fi
}

# Detect Linux distribution
detect_distro() {
    if [[ -f "/etc/os-release" ]]; then
        source /etc/os-release
        echo "$ID"
    else
        echo "unknown"
    fi
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
    elif command_exists dnf; then
        log_info "Installing $package with dnf..."
        sudo dnf install -y "$package"
    elif command_exists pacman; then
        log_info "Installing $package with pacman..."
        sudo pacman -S --noconfirm "$package"
    elif command_exists zypper; then
        log_info "Installing $package with zypper..."
        sudo zypper install -y "$package"
    elif command_exists brew; then
        log_info "Installing $package with brew..."
        brew install "$package"
    else
        log_error "No supported package manager found. Please install $package manually."
        return 1
    fi
}

# Check if running as root
is_root() {
    [[ $EUID -eq 0 ]]
}

# Ensure directory exists
ensure_dir() {
    local dir=$1
    if [[ ! -d "$dir" ]]; then
        log_info "Creating directory: $dir"
        mkdir -p "$dir"
    fi
}
