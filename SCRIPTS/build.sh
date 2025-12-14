#!/bin/bash
# ============================================================================
# NaoKernel Build Script
# ============================================================================

set -e  # Exit on error

# Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ============================================================================
# Build Functions
# ============================================================================

build_kernel() {
    log_step "Starting kernel build process..."
    
    # Ensure bin directory exists
    ensure_dir "bin"
    
    # Compile kernel assembly
    log_build "Compiling kernel assembly (kernel.asm)..."
    nasm -f elf32 kernel.asm -o bin/kasm.o
    log_success "Kernel assembly compiled"
    
    # Compile output subsystem
    log_build "Compiling output subsystem (output/output.c)..."
    gcc -fno-stack-protector -m32 -c output/output.c -o bin/output.o
    log_success "Output subsystem compiled"
    
    # Compile input subsystem
    log_build "Compiling input subsystem (input/input.c)..."
    gcc -fno-stack-protector -m32 -c input/input.c -o bin/input.o
    log_success "Input subsystem compiled"
    
    # Compile shell
    log_build "Compiling shell (shell/shell.c)..."
    gcc -fno-stack-protector -m32 -c shell/shell.c -o bin/shell.o
    log_success "Shell compiled"

    # Compile format
    log_build "Compiling format module (fs/format/format.c)..."
    gcc -fno-stack-protector -m32 -c fs/format/format.c -o bin/format.o
    log_success "Format module compiled"
    
    # Compile kernel
    log_build "Compiling kernel (kernel.c)..."
    gcc -fno-stack-protector -m32 -c kernel.c -o bin/kc.o
    log_success "Kernel compiled"

    # Compile File System and Tokenizer (if any)
    log_build "Compiling file system (fs/fs.c)..."
    gcc -fno-stack-protector -m32 -c fs/fs.c -o bin/fs.o
    log_success "File system compiled"
    log_build "Compiling tokenizer (tokenizer/tokenizer.c)..."
    gcc -fno-stack-protector -m32 -c shell/tokenizer.c -o bin/tokenizer.o
    log_success "Tokenizer compiled"
    
    # Link everything together
    log_build "Linking kernel..."
    ld -m elf_i386 -T link.ld -o bin/kernel bin/kasm.o bin/kc.o bin/output.o bin/input.o bin/shell.o bin/fs.o bin/format.o bin/tokenizer.o
    log_success "Kernel linked successfully"
    
    log_success "Build complete! Kernel binary: bin/kernel"
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "================================================"
    log_info "NaoKernel Build System"
    log_info "================================================"
    log_info ""
    
    build_kernel
    
    # Run in QEMU if --run argument is provided
    if [[ "$1" == "--run" ]]; then
        log_info ""
        log_info "Starting NaoKernel..."
        qemu-system-i386 -kernel bin/kernel
    fi
}

main "$@"
