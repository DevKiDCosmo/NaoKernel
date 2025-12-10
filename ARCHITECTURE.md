# NaoKernel Architecture

## Overview

NaoKernel is organized into modular subsystems, each with clear responsibilities and interfaces. This document describes the architecture and how the components interact.

## Core Subsystems

### 1. Input Subsystem (`input/`)

**Purpose**: Handle all keyboard input and line buffering operations.

**Key Components**:
- `input.c` / `input.h` - Input handling implementation
- `InputBuffer` structure - Stores and manages input state
- Keyboard interrupt processing
- Line editing (character input, backspace)

**Key Functions**:
- `input_handle_keyboard()` - Process keyboard interrupts
- `input_getline()` - Blocking function to read a line of input
- `input_add_char()` - Add character to buffer with echo
- `input_backspace()` - Handle backspace key
- String utilities: `strcmp_custom()`, `strlen_custom()`, etc.

**Interface**: Called by kernel interrupt handler and shell for input operations.

---

### 2. Output Subsystem (`output/`)

**Purpose**: Manage all screen output and cursor positioning.

**Key Components**:
- `output.c` / `output.h` - Output handling implementation
- Direct VGA text mode access (0xB8000)
- Cursor position tracking

**Key Functions**:
- `kprint()` - Print string to screen
- `kprint_newline()` - Move to next line
- `kprint_char()` - Print single character
- `kprint_hex()` - Print hexadecimal number
- `kprint_dec()` - Print decimal number
- `clear_screen()` - Clear entire screen
- `get_cursor_position()` / `set_cursor_position()` - Cursor management

**Interface**: Used by kernel and shell for all output operations.

---

### 3. Shell Subsystem (`shell/`)

**Purpose**: Provide command-line interface and command execution.

**Key Components**:
- `shell.c` / `shell.h` - Shell implementation
- Command parser
- Command implementations

**Key Functions**:
- `nano_shell()` - Main shell loop
- `shell_execute_command()` - Parse and execute commands
- `shell_handle_keyboard()` - Keyboard event handler (delegates to input subsystem)
- Command implementations:
  - `cmd_help()` - Show help
  - `cmd_clear()` - Clear screen
  - `cmd_echo()` - Echo text
  - `cmd_about()` - System info
  - `cmd_exit()` - Shutdown

**Interface**: Called by kernel on startup; handles keyboard events from interrupt handler.

---

### 4. Kernel Core (`kernel.c`, `kernel.asm`)

**Purpose**: Core kernel initialization, interrupt handling, and hardware management.

**Key Components**:
- Interrupt Descriptor Table (IDT) setup
- Keyboard controller initialization
- Interrupt handlers
- Main kernel entry point

**Key Functions**:
- `kmain()` - Kernel entry point
- `idt_init()` - Initialize interrupt system
- `kb_init()` - Initialize keyboard controller
- `keyboard_handler_main()` - Process keyboard interrupts

**Interface**: Entry point for system; coordinates all subsystems.

---

## Data Flow

### Keyboard Input Flow
```
1. Keyboard interrupt triggered
2. keyboard_handler (ASM) → keyboard_handler_main (C)
3. keyboard_handler_main → shell_handle_keyboard
4. shell_handle_keyboard → input_handle_keyboard
5. input_handle_keyboard processes keystroke:
   - Regular chars: Add to buffer, echo via output subsystem
   - Enter: Mark input complete, trigger command execution
   - Backspace: Remove char from buffer, clear on screen
6. Shell reads completed input and executes command
```

### Command Execution Flow
```
1. nano_shell() calls input_getline() (blocking)
2. User types command and presses Enter
3. input_getline() returns completed command string
4. shell_execute_command() parses command
5. Appropriate cmd_* function called
6. Command uses output subsystem for display
7. Shell loops back to input_getline()
```

---

## Design Principles

### Separation of Concerns
Each subsystem has a single, well-defined responsibility:
- Input handles keyboard and buffering
- Output handles display
- Shell handles command execution
- Kernel handles hardware and coordination

### Minimal Coupling
Subsystems communicate through clean interfaces:
- Kernel doesn't know about command implementations
- Shell doesn't implement input/output directly
- Input/Output subsystems are reusable

### No Filesystem
By design, NaoKernel does not implement a filesystem. All functionality is in-memory only. This keeps the kernel minimal and focused on core OS concepts.

---

## Adding New Commands

To add a new command to the shell:

1. Implement command function in `shell/shell.c`:
```c
void cmd_mycommand(char *args)
{
    kprint("My command output\n");
}
```

2. Add command check in `shell_execute_command()`:
```c
if (strcmp_custom(cmd, "mycommand") == 0) {
    cmd_mycommand(cmd + 9);  /* skip command name */
    return;
}
```

3. Update help text in `cmd_help()`:
```c
kprint("  mycommand - Description\n");
```

4. Document in `shell/COMMANDS.md`

---

## Memory Layout

- **Video Memory**: 0xB8000 - 0xB8FA0 (25 lines × 80 cols × 2 bytes)
- **Kernel Code**: Loaded at 0x100000 (1MB mark) via linker script
- **Stack**: Set up by bootloader
- **Input Buffer**: Static buffer in input subsystem (256 bytes)

---

## Future Extensions

Possible enhancements while maintaining the no-filesystem constraint:

1. **More shell commands**: calculator, memory viewer, system info
2. **Color support**: Extended output functions for colored text
3. **Command history**: Up/down arrow to recall previous commands
4. **Tab completion**: Auto-complete command names
5. **Multi-line input**: Support for longer inputs
6. **Scrolling**: Handle output that exceeds screen size
7. **Mouse support**: Basic mouse cursor and clicking
