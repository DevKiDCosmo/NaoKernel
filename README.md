# Nao Kernel

What I want:
- An custom bootloader
- An custom kernel
- A functional operating system with filesystem
- Services and background processesw

Disclaimer: I use AI for the bare minium thing (I just didn't get the hang out of it.). Afterwards I code entire structures myself.

A minimal x86 kernel with an integrated shell.

## Features

- **Modular Architecture**: Separated input, output, and shell subsystems
- **Interactive Shell**: Command-line interface with multiple commands
- **Keyboard Input**: Full keyboard support with line editing
- **Screen Output**: Text mode display with cursor management

## Project Structure

```
NaoKernel/
├── input/           # Input subsystem (keyboard handling, line buffering)
│   ├── input.c
│   └── input.h
├── output/          # Output subsystem (screen output, cursor control)
│   ├── output.c
│   └── output.h
├── shell/           # Shell implementation (command parsing and execution)
│   ├── shell.c
│   ├── shell.h
│   └── COMMANDS.md  # Shell command documentation
├── kernel.c         # Kernel initialization and interrupt handling
├── kernel.asm       # Assembly code for low-level operations
├── keyboard_map.h   # Keyboard scancode mapping
├── link.ld          # Linker script
└── build.sh         # Build script
```

## Build your own kernel

With `bash install.sh` everything needed for building is installed.

Just run `bash build.sh`

For making it into an .iso or to a bootable .usb drive run `bash flash.sh`

## Shell Commands

The kernel includes an interactive shell with the following commands:

- `help` - Show available commands
- `clear` - Clear the screen
- `echo` - Echo text to screen
- `about` - Show system information
- `exit` - Shutdown the system

See `shell/COMMANDS.md` for detailed command documentation.

## Architecture

### Input Subsystem
Handles keyboard input, scan code processing, line buffering, and input editing features like backspace.

### Output Subsystem  
Manages screen output, cursor positioning, and text formatting functions for displaying information.

### Shell
Provides a command-line interface with command parsing, execution, and extensibility for adding new commands.

This modular design separates concerns and makes the codebase easier to understand and extend.


# Execution

## Without Disk

```bash
bash build.sh --run
```

## With Disk
```bash 
bash build.sh && bash run.sh --clear
```