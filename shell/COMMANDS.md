# NaoKernel Shell Commands

## Available Commands

### help
Shows a list of available commands with brief descriptions.

**Usage:** `help`

### clear
Clears the screen and resets the cursor to the top.

**Usage:** `clear`

### echo
Prints the given text to the screen.

**Usage:** `echo <text>`

**Example:**
```
> echo Hello, World!
Hello, World!
```

### about
Displays system information about NaoKernel.

**Usage:** `about`

### exit
Shuts down the kernel/system.

**Usage:** `exit`

### disk
Manages disk drives and partitions.

**Usage:** `disk <subcommand>`

**Subcommands:**
- `list` - Lists all detected drives
- `mount` - Mounts a disk (not yet implemented)

**Example:**
```
> disk list
Listing disks...
Detected Drives:
 Drive 0: Primary Master
```

## Command Line Features

- **Line editing**: Type commands and use backspace to correct mistakes
- **Command prompt**: The shell displays a `> ` prompt before each command
- **Case sensitive**: All commands are lowercase

## Architecture

The shell is built on modular subsystems:

- **Input subsystem** (`input/`): Handles keyboard input and line buffering
- **Output subsystem** (`output/`): Manages screen output and cursor positioning
- **Shell** (`shell/`): Command parsing and execution

This modular design makes it easy to add new commands or extend functionality.
