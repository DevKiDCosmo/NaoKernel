# NaoKernel Input System

## Overview
The shell code has been refactored into a reusable input system that allows you to easily create shells, REPLs, and other interactive applications.

## Key Components

### 1. InputBuffer Structure
```c
typedef struct {
    char buffer[MAX_INPUT_LENGTH];  // Stores the input
    int position;                    // Current position in buffer
    int ready;                       // Flag: is input complete?
    char *prompt;                    // Prompt to display
} InputBuffer;
```

### 2. Core Functions

#### Initialization
```c
void input_init(InputBuffer *inp, char *prompt);
```
Initialize an input buffer with a custom prompt (e.g., "> ", "$ ", ">>> ")

#### Getting Input
```c
char* input_getline(InputBuffer *inp);
```
Blocking function that:
- Displays the prompt
- Waits for user to press Enter
- Returns the complete line of input

#### Other Functions
```c
void input_reset(InputBuffer *inp);           // Clear buffer for reuse
void input_print_prompt(InputBuffer *inp);    // Display prompt
void input_add_char(InputBuffer *inp, char c); // Add character (internal)
void input_backspace(InputBuffer *inp);        // Handle backspace (internal)
void input_complete(InputBuffer *inp);         // Mark input ready (internal)
```

### 3. String Utilities
```c
int strcmp_custom(const char *s1, const char *s2);  // Compare strings
int strlen_custom(const char *str);                 // Get string length
```

## Usage Examples

### Basic Shell
```c
InputBuffer input;
char *line;

input_init(&input, "> ");

while (1) {
    line = input_getline(&input);  // Blocks until Enter
    
    // Process the line
    if (strcmp_custom(line, "exit") == 0) {
        break;
    }
    
    kprint("You typed: ");
    kprint(line);
    kprint_newline();
}
```

### Custom REPL
```c
InputBuffer repl;
input_init(&repl, ">>> ");

while (running) {
    char *cmd = input_getline(&repl);
    execute_command(cmd);
}
```

### Different Prompts
```c
InputBuffer input;

// Login prompt
input_init(&input, "Username: ");
char *user = input_getline(&input);

// Change prompt for password
input.prompt = "Password: ";
char *pass = input_getline(&input);
```

### Interactive Form
```c
InputBuffer input;

input_init(&input, "Name: ");
char *name = input_getline(&input);

input.prompt = "Email: ";
char *email = input_getline(&input);

input.prompt = "Age: ";
char *age = input_getline(&input);
```

## Features

### Automatic Features
- ✓ Prompt display before each input
- ✓ Character echoing to screen
- ✓ Backspace support (deletes character)
- ✓ Enter key handling (completes input)
- ✓ Newline after Enter
- ✓ Buffer overflow protection

### Built-in Key Handling
- **Enter** - Completes input, triggers newline
- **Backspace** - Deletes previous character
- **Regular keys** - Adds to buffer and displays

## Architecture

```
Keyboard Interrupt
       ↓
keyboard_handler_main() [kernel.c]
       ↓
shell_handle_keyboard() [shell.c]
       ↓
   ┌──────────────┐
   │ Enter Key?   │ → input_complete()
   ├──────────────┤
   │ Backspace?   │ → input_backspace()
   ├──────────────┤
   │ Regular Char │ → input_add_char()
   └──────────────┘
       ↓
  InputBuffer updated
       ↓
  input_getline() returns
```

## Extending the System

### Add New Key Handlers
Edit `shell_handle_keyboard()` in shell.c:
```c
void shell_handle_keyboard(char keycode)
{
    if (keycode == MY_SPECIAL_KEY) {
        // Handle special key
        return;
    }
    // ... existing code
}
```

### Create Custom Input Processing
```c
char* my_getline_no_echo(InputBuffer *inp) {
    // Modify to not echo characters (for passwords)
}
```

### Add Line Editing
```c
void input_move_cursor_left(InputBuffer *inp);
void input_move_cursor_right(InputBuffer *inp);
void input_delete_char_at(InputBuffer *inp, int pos);
```

## File Structure

- `shell.h` - Header with function declarations and InputBuffer struct
- `shell.c` - Implementation of input system and shell commands
- `kernel.c` - Kernel that uses the shell system
- `USAGE_EXAMPLES.c` - Code examples for various use cases

## Benefits of Refactoring

1. **Reusability** - Use anywhere in kernel for input
2. **Modularity** - Separate concerns (input vs command processing)
3. **Extensibility** - Easy to add features like history, tab completion
4. **Clean API** - Simple functions with clear purposes
5. **Flexibility** - Custom prompts, different contexts

## Future Enhancements

Possible additions:
- Command history (up/down arrows)
- Tab completion
- Line editing (arrow keys, home, end)
- Input validation
- Password mode (no echo)
- Multi-line input
- Syntax highlighting
