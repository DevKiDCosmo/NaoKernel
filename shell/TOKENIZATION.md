# Argument Tokenization and Parsing

## Overview
The NaoKernel shell now features an advanced tokenization system for proper argument parsing with support for quotes, escape sequences, and special characters.

## Features

### 1. **Tokenization Function** (`tokenize()`)
The main tokenization function handles complex argument parsing:
- **Quoted strings**: Preserves spaces within single (`'`) or double (`"`) quotes
- **Escape sequences**: Supports `\"`, `\'`, `\\`, `\n`, `\t`
- **Mixed quotes**: Can use single quotes inside double quotes and vice versa
- **Multiple whitespace**: Treats consecutive spaces/tabs as a single separator

### 2. **Special Character Detection** (`has_special_chars()`)
Detects if a string contains special characters that require proper tokenization:
- Quotes: `"` `'`
- Escapes: `\`
- Pipes and redirects: `|` `>` `<`
- Command separators: `&` `;`
- Variable expansion: `$` `` ` ``
- Grouping: `(` `)` `{` `}` `[` `]`
- Wildcards: `*` `?`

### 3. **Helper Functions**
- `get_command_token()`: Returns the first token (command name)
- `get_arg_token()`: Returns the nth argument (0-indexed after command)

## Usage

### For Commands That Need Tokenization
Commands like `disk` that require proper argument parsing:

```c
void cmd_disk(char *args)
{
    TokenArray tokens;
    tokenize(args, &tokens);
    
    if (tokens.count == 0) {
        kprint("Usage: disk <list|mount>\n");
        return;
    }
    
    // Access tokenized arguments
    const char *subcmd = tokens.tokens[0];
    
    // Check for additional arguments
    if (tokens.count > 1) {
        const char *device = tokens.tokens[1];
        // Process device...
    }
}
```

### For Commands That Don't Need Tokenization
Commands like `echo` that should output raw text receive untokenized arguments:

```c
void cmd_echo(char *args)
{
    if (args[0] != '\0') {
        kprint(args);  // Raw output, no parsing
    }
    kprint_newline();
}
```

## Command Configuration
In the command map, specify whether a command needs tokenization:

```c
static Command command_map[] = {
    {"echo", (void*)cmd_echo, 1, 0, "Echo text"},     // No tokenization
    {"disk", (void*)cmd_disk, 1, 1, "Disk command"},  // Uses tokenization
    // ...
};
```

The fourth parameter (`needs_tokenization`) controls parsing behavior:
- `0`: Pass raw arguments (for echo-like commands)
- `1`: Use tokenization for proper parsing

## Examples

### With Quotes
```
> disk mount "/dev/disk 1"
```
Tokenizes to:
- `tokens[0]` = "mount"
- `tokens[1]` = "/dev/disk 1" (space preserved within quotes)

### With Escape Sequences
```
> command "Line 1\nLine 2"
```
Tokenizes to:
- `tokens[0]` = "Line 1\nLine 2" (with actual newline character)

### Echo Without Tokenization
```
> echo Hello   "World"
```
Output: `Hello   "World"` (raw, quotes and spaces preserved)

## Benefits
1. **Proper argument handling**: Quotes and escapes work as expected
2. **Flexible parsing**: Commands can choose tokenization vs raw input
3. **Future-ready**: Foundation for pipes, redirects, and variables
4. **Kernel-safe**: No external dependencies (memcpy-free implementation)
