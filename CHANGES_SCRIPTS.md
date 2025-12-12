# Shell Scripts Reorganization

## Summary

All shell scripts have been rewritten with universal logging and moved to the `SCRIPTS/` directory. Only `start.sh` remains in the root as the main entry point.

## Changes Made

### 1. New SCRIPTS Directory Structure

```
SCRIPTS/
├── lib.sh              # Universal logging and utility library
├── build.sh            # Native kernel build script
├── docker-build.sh     # Docker-based build (Ubuntu compilation)
├── install.sh          # Dependency installation (Ubuntu/Debian only)
├── run.sh              # Kernel execution in QEMU
├── setup-drives.sh     # Drive image setup
├── README.md           # Documentation for all scripts
└── old/                # Archive of previous scripts
    ├── build.sh
    ├── docker.sh
    ├── drives.sh
    ├── install.sh
    ├── run.sh
    └── start.sh
```

### 2. Universal Logging System

All scripts now use consistent logging via `SCRIPTS/lib.sh`:

- **Color-coded output**: Blue (INFO), Green (SUCCESS), Yellow (WARNING), Red (ERROR), Cyan (STEP), Magenta (BUILD)
- **Consistent format**: All messages use standardized log functions
- **Shared utilities**: OS detection, package management, command checking

### 3. Replaced Scripts

#### Removed from Root:
- `docker.sh` → `SCRIPTS/docker-build.sh` (with logging)
- `install.sh` → `SCRIPTS/install.sh` (Ubuntu/Debian only, with logging)
- `run.sh` → `SCRIPTS/run.sh` (with logging)
- `docker/build.sh` → `SCRIPTS/build.sh` (with logging)
- `docker/drives.sh` → `SCRIPTS/setup-drives.sh` (with logging)

#### Rewritten:
- `start.sh` - Completely rewritten as universal entry point with logging

### 4. New start.sh Features

The new `start.sh` is a universal entry point with:

- **OS Detection**: Automatically detects macOS, Linux, Docker environment
- **Dependency Checking**: Verifies gcc, nasm, qemu availability
- **Multiple Actions**: 
  - `--build, -b` - Build kernel
  - `--run, -r` - Build and run kernel
  - `--clean, -c` - Clean build artifacts
  - `--install, -i` - Install dependencies (Ubuntu/Debian)
  - `--setup-drives` - Setup disk images
  - `--docker-build` - Force Docker build
  - `--help, -h` - Show help
- **Universal Logging**: All operations use color-coded logging
- **Smart Build Selection**: Uses Docker build on Ubuntu, native on macOS

### 5. Benefits

1. **Cleaner Root Directory**: Only one script in root
2. **Universal Logging**: All operations clearly logged with colors
3. **Better Organization**: All scripts in dedicated directory
4. **Maintained History**: Old scripts archived for reference
5. **Consistent Interface**: All scripts follow same logging pattern
6. **Cross-Platform**: Works on macOS, Linux, Docker

### 6. Updated Files

- `Dockerfile` - Updated to use `SCRIPTS/build.sh`
- All scripts now source `SCRIPTS/lib.sh` for utilities

## Usage

### Main Entry Point (Recommended)
```bash
./start.sh --help          # Show all options
./start.sh --build         # Build kernel
./start.sh --run           # Build and run
./start.sh --install       # Install dependencies
```

### Direct Script Usage
```bash
bash SCRIPTS/build.sh           # Build directly
bash SCRIPTS/docker-build.sh    # Build with Docker
bash SCRIPTS/install.sh         # Install deps
bash SCRIPTS/run.sh             # Run kernel
bash SCRIPTS/setup-drives.sh    # Setup drives
```

## Migration Notes

- Old scripts are preserved in `SCRIPTS/old/` for reference
- All functionality maintained, just reorganized
- New universal logging throughout
- `build.sh` now has proper step-by-step logging instead of just printing
- Installation script explicitly checks for Ubuntu/Debian
- Docker build script properly uses new structure

## Technical Details

### Logging Functions
- `log_info()` - General information (blue)
- `log_success()` - Success messages (green)
- `log_warning()` - Warnings (yellow)
- `log_error()` - Errors (red)
- `log_step()` - Major steps (cyan)
- `log_build()` - Build operations (magenta)

### Utility Functions
- `command_exists()` - Check if command is available
- `detect_os()` - Detect operating system
- `detect_distro()` - Detect Linux distribution
- `install_package()` - Universal package installation
- `ensure_dir()` - Create directory if needed

## Testing

All scripts have been tested for:
- ✅ Proper loading of lib.sh
- ✅ Universal logging output
- ✅ Executable permissions
- ✅ Help text display
- ✅ Build process logging

Note: macOS native linking may require adjustments due to ld differences. Use Docker build on macOS if needed.
