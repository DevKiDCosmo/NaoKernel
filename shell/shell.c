/*
 * NaoKernel Shell
 * Command-line interface with multiple commands
 */

#include "../input/input.h"
#include "../output/output.h"
#include "../essentials/types.h"
#include "tokenizer.h"
#include "../fs/fs.h"

/* External global filesystem map from kernel */
extern FilesystemMap global_fs_map;

/* Shell state */
static InputBuffer input;
static CommandHistory history;
static int shell_running = 1;

/* Command function pointer types */
typedef void (*CommandFunc)(void);
typedef void (*CommandFuncWithArgs)(char *);
typedef void (*CommandFuncWithTokens)(TokenArray *);

/* Command structure */
typedef struct
{
	const char *name;
	void *func;
	int takes_argument;
	int needs_tokenization; /* 1 if command needs proper tokenization, 0 for raw args */
	const char *description;
} Command;

static Command command_map[];

/* Forward declarations of helper functions */
static char *skip_spaces(char *str);
static char *find_space(char *str);
static int strlen_custom(const char *str);
static char to_lower(char c);
static int strncmp_case_insensitive(const char *s1, const char *s2, int n);

/* Command implementations */

/* Help command - show available commands */
void cmd_help(void)
{
	kprint("Available commands:\n");
	int i;
	Command *cmd;
	for (i = 0; command_map[i].name != 0; i++)
	{
		cmd = &command_map[i];
		kprint(" - ");
		kprint(cmd->name);
		kprint(": ");
		kprint(cmd->description);
		kprint_newline();
	}
}

/* Clear command - clear the screen */
void cmd_clear(void)
{
	clear_screen();
}

/* Echo command - print arguments */
void cmd_echo(char *args)
{
	if (args[0] != '\0')
	{
		kprint(args);
	}
	kprint_newline();
}

/* About command - system information */
void cmd_about(void)
{
	kprint("NaoKernel v0.1\n");
	kprint("A minimal x86 kernel with shell\n");
	kprint("(c) 2026 by Duy Nam Schlitz\n");
}

/* Exit command - shutdown */
void cmd_exit(void)
{
	kprint("Shutting down...\n");
	shell_running = 0;

	// Send system signal for shutdown if needed
	// send_signal(SIGUSR1);
}

void cmd_test(void)
{
	kprint("Test command executed successfully.\n");
}

void cmd_disk(char *args)
{
	/* Skip leading spaces */
	char *subcmd = skip_spaces(args);
	
	/* Empty command */
	if (subcmd[0] == '\0')
	{
		kprint("Usage: disk <list|mount>\n");
		return;
	}

	/* Find end of subcommand */
	char *subcmd_end = find_space(subcmd);
	int subcmd_len = subcmd_end - subcmd;

	if (strncmp_case_insensitive(subcmd, "list", subcmd_len) == 0 && subcmd_len == 4)
	{
		fs_list(&global_fs_map);
	}
	else if (strncmp_case_insensitive(subcmd, "mount", subcmd_len) == 0 && subcmd_len == 5)
	{
		kprint("Mounting disk...\n");
		/* TODO: Implement disk mounting */

		/* Get additional arguments */
		char *device_args = skip_spaces(subcmd_end);
		if (device_args[0] != '\0')
		{
			kprint("  Device: ");
			kprint(device_args);
			kprint_newline();
		}
	}
	else
	{
		kprint("Unknown disk command. Use 'list' or 'mount'.\n");
	}
	return;
}

void cmd_history(void)
{
	kprint("Command History:\n");
	int i;
	for (i = 0; i < history.count; i++)
	{
		char num_str[16];
		int num = i + 1;
		int len = 0;
		int temp = num;

		// Add additional spacing for alignment

		/* Convert number to string */
		if (temp == 0)
		{
			num_str[0] = '0';
			len = 1;
		}
		else
		{
			while (temp > 0)
			{
				num_str[len++] = '0' + (temp % 10);
				temp /= 10;
			}
			/* Reverse the string */
			int j;
			for (j = 0; j < len / 2; j++)
			{
				char tmp = num_str[j];
				num_str[j] = num_str[len - 1 - j];
				num_str[len - 1 - j] = tmp;
			}
		}
		num_str[len] = '\0';

		/* Print: number. command */
		kprint(num_str);
		kprint(". ");

		/* Print command in red if invalid, white if valid */
		if (history.valid[i])
		{
			kprint(history.commands[i]);
		}
		else
		{
			kprint_colored(history.commands[i], 0x04); /* Red text */
		}
		kprint_newline();
	}
}

/* Command map - array of all available commands */
static Command command_map[] = {
	{"help", (void *)cmd_help, 0, 0, "Show available commands"},
	{"clear", (void *)cmd_clear, 0, 0, "Clear the screen"},
	{"echo", (void *)cmd_echo, 1, 0, "Echo text to screen"}, /* No tokenization for echo - raw output */
	{"about", (void *)cmd_about, 0, 0, "Show system information"},
	{"exit", (void *)cmd_exit, 0, 0, "Shutdown the system"},
	{"test", (void *)cmd_test, 0, 0, "Run a test command"},
	{"history", (void *)cmd_history, 0, 0, "Show command history"},
	{"disk", (void *)cmd_disk, 1, 0, "Disk operations (list, mount)"},
	{0, 0, 0, 0, 0}									  /* Sentinel entry */
};

/* Skip leading whitespace */
static char *skip_spaces(char *str)
{
	while (*str == ' ' || *str == '\t')
	{
		str++;
	}
	return str;
}

/* Find the first space in a string and return pointer to it */
static char *find_space(char *str)
{
	while (*str != '\0' && *str != ' ' && *str != '\t')
	{
		str++;
	}
	return str;
}

/* Calculate string length */
static int strlen_custom(const char *str)
{
	int len = 0;
	while (str[len] != '\0')
	{
		len++;
	}
	return len;
}

/* Convert character to lowercase */
static char to_lower(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c + ('a' - 'A');
	}
	return c;
}

/* Case-insensitive string comparison */
static int strncmp_case_insensitive(const char *s1, const char *s2, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		char c1 = to_lower(s1[i]);
		char c2 = to_lower(s2[i]);
		if (c1 != c2 || c1 == '\0' || c2 == '\0')
		{
			return (unsigned char)c1 - (unsigned char)c2;
		}
	}
	return 0;
}

/* Parse and execute shell commands - returns 1 if command found, 0 if not */
int shell_execute_command(char *command)
{
	char *cmd_start;
	char *cmd_end;
	char *args;
	int cmd_len;
	int i;
	Command *cmd;

	/* Skip leading spaces */
	cmd_start = skip_spaces(command);

	/* Empty command */
	if (cmd_start[0] == '\0')
	{
		return 0; /* Don't count empty commands as valid */
	}

	/* Find end of command (first space or end of string) */
	cmd_end = find_space(cmd_start);
	cmd_len = cmd_end - cmd_start;

	/* Get arguments (everything after first space) */
	args = skip_spaces(cmd_end);

	/* Search through command map */
	for (i = 0; command_map[i].name != 0; i++)
	{
		cmd = &command_map[i];

		/* Check if command name matches (case-insensitive) */
		if (strncmp_case_insensitive(cmd_start, cmd->name, cmd_len) == 0 &&
			strlen_custom(cmd->name) == cmd_len)
		{

			/* Execute command based on whether it takes arguments */
			if (cmd->takes_argument)
			{
				/* Call with arguments */
				((CommandFuncWithArgs)cmd->func)(args);
			}
			else
			{
				/* Call without arguments */
				((CommandFunc)cmd->func)();
			}
			return 1; /* Command found and executed */
		}
	}

	/* Unknown command - print just the command name */
	kprint("Unknown command: ");
	/* Temporarily null-terminate command for printing */
	char temp = *cmd_end;
	*cmd_end = '\0';
	kprint(cmd_start);
	*cmd_end = temp;
	kprint_newline();
	kprint("Type 'help' for available commands.\n");
	return 0; /* Command not found */
}

/* Shell main loop */
void nano_shell(void)
{
	char *line;
	int command_valid;

	kprint("\n=== NaoKernel Shell ===\n");
	kprint("Type 'help' for available commands.\n");
	kprint("Use UP/DOWN arrows to browse command history.\n\n");

	/* Initialize input system with prompt */
	input_init(&input, "> ");

	/* Initialize command history */
	history_init(&history);

	/* Share history with input system for arrow key navigation */
	input_set_history(&history);

	while (shell_running)
	{
		/* Get line of input (blocks until Enter is pressed) */
		line = input_getline(&input);

		/* Execute command and check if it was valid */
		command_valid = shell_execute_command(line);

		/* Add all non-empty commands to history (both valid and invalid) */
		if (line[0] != '\0')
		{
			history_add(&history, line, command_valid);
			/* Update input system's history */
			input_set_history(&history);
		}
	}
}

/* Handle keyboard input for shell */
void shell_handle_keyboard(char keycode)
{
	/* Delegate to input system */
	input_handle_keyboard(keycode);
}
