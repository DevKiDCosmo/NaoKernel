# NaoKernel Scripts

This directory contains all shell scripts for building, installing, and running NaoKernel.

## Universal Logging

All scripts use the shared logging library (`lib.sh`) which provides:
- Color-coded output for better readability
- Consistent log levels: INFO, SUCCESS, WARNING, ERROR, STEP, BUILD
- Universal utility functions

## Scripts Overview

### Core Scripts

- **lib.sh** - Shared library with logging and utility functions
  - Universal logging functions (log_info, log_success, log_error, etc.)
  - OS detection (detect_os, detect_distro)
  - Package management (install_package)
  - Command existence checks (command_exists)

- **build.sh** - Native kernel build script
  - Compiles kernel assembly, C code, and all subsystems
  - Links final kernel binary
  - Universal logging throughout build process
  - Usage: `bash SCRIPTS/build.sh [--run]`

- **docker-build.sh** - Docker-based build (Ubuntu compilation)
  - Builds Docker image
  - Compiles kernel inside Ubuntu container
  - Ensures Ubuntu-specific compilation requirements
  - Usage: `bash SCRIPTS/docker-build.sh`

- **install.sh** - Dependency installation (Ubuntu/Debian only)
  - Checks for Ubuntu/Debian system
  - Installs build tools, gcc, nasm, qemu
  - Usage: `bash SCRIPTS/install.sh`

- **run.sh** - Kernel execution in QEMU
  - Launches kernel with disk and floppy images
  - Auto-sets up drives if missing
  - Usage: `bash SCRIPTS/run.sh`

- **setup-drives.sh** - Drive image setup
  - Creates disk.img (10MB hard disk)
  - Creates floppy.img (1.44MB floppy)
  - Formats with FAT filesystem
  - Usage: `bash SCRIPTS/setup-drives.sh`

## Usage Examples

### Build kernel natively
```bash
bash SCRIPTS/build.sh
```

### Build kernel in Docker (Ubuntu)
```bash
bash SCRIPTS/docker-build.sh
```

### Install dependencies (Ubuntu/Debian)
```bash
bash SCRIPTS/install.sh
```

### Run kernel
```bash
bash SCRIPTS/run.sh
```

### Setup disk images
```bash
bash SCRIPTS/setup-drives.sh
```

## Main Entry Point

Use the root `start.sh` script as the main entry point:

```bash
./start.sh --build      # Build kernel
./start.sh --run        # Build and run
./start.sh --install    # Install dependencies
./start.sh --help       # Show all options
```

## Logging Colors

- **BLUE** - [INFO] - General information
- **GREEN** - [SUCCESS] - Successful operations
- **YELLOW** - [WARNING] - Warnings and non-critical issues
- **RED** - [ERROR] - Errors and failures
- **CYAN** - [STEP] - Major build/process steps
- **MAGENTA** - [BUILD] - Build-specific operations

## OS Support

- **Linux (Ubuntu/Debian)** - Full support with automatic dependency installation
- **Linux (Other)** - Build support, manual dependency installation
- **macOS** - Full support with Homebrew
- **Docker** - Full support for containerized builds

## Old Scripts

Previous scripts are archived in `SCRIPTS/old/` for reference.
