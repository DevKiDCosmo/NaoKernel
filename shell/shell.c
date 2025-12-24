/*
 * NaoKernel Shell
 * Command-line interface with multiple commands
 */

#include "../input/input.h"
#include "../output/output.h"
#include "../essentials/types.h"
#include "tokenizer.h"
#include "../fs/fs.h"
#include "../fs/format/format.h"
#include "../fs/mount/mount.h"

/* External global filesystem map from kernel */
extern FilesystemMap global_fs_map;
extern MountTable global_mount_table;

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
static void strcpy_local(char *dest, const char *src);

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
		kprint("Usage: disk <list|mount|format>\n");
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
		char *device_args = skip_spaces(subcmd_end);
		
		if (device_args[0] == '\0')
		{
			kprint("Usage: disk mount <device>\n");
			kprint("Example: disk mount ide0\n");
			return;
		}

		/* Find the drive by ID - check for exact match */
		int pos = -1;
		if (strncmp_case_insensitive(device_args, "ide0", 4) == 0 && 
		    (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 0;
		} else if (strncmp_case_insensitive(device_args, "ide1", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 1;
		} else if (strncmp_case_insensitive(device_args, "ide2", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 2;
		} else if (strncmp_case_insensitive(device_args, "ide3", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 3;
		} else {
			kprint("Unknown device specified.\n");
			return;
		}

		DriveInfo *drive = &global_fs_map.drives[pos];
		
		if (!drive->present) {
			kprint("Drive not present.\n");
			return;
		}

		kprint("Mounting ");
		kprint(drive->idNAME);
		kprint("...\n");

		/* Check if formatted first */
		if (!is_drive_formatted(drive)) {
			kprint("Error: Drive is not formatted.\n");
			kprint("Use 'disk format ");
			kprint(drive->idNAME);
			kprint(" <fs_type>' to format it first.\n");
			return;
		}

		/* Display detected filesystem */
		kprint("  Filesystem: ");
		switch (drive->fs_type) {
			case FS_TYPE_FAT12: kprint("FAT12"); break;
			case FS_TYPE_FAT16: kprint("FAT16"); break;
			case FS_TYPE_FAT32: kprint("FAT32"); break;
			default: kprint("Unknown"); break;
		}
		kprint("\n");

		MountResult result = mount_drive(&global_mount_table, drive);
		if (result == MOUNT_SUCCESS) {
			kprint("Mount successful!\n");
			
			/* Update the prompt for current drive */
			const char *new_prompt = get_current_prompt(&global_mount_table);
			input_set_prompt(&input, (char *)new_prompt);
		} else {
			kprint("Mount failed: ");
			kprint(get_mount_result_string(result));
			kprint("\n");
		}
	}
	else if (strncmp_case_insensitive(subcmd, "dismount", subcmd_len) == 0 && subcmd_len == 8)
	{
		/* Dismount the currently active drive */
		if (global_mount_table.current_mount == -1) {
			kprint("No drive is currently mounted.\n");
			return;
		}

		int mount_idx = global_mount_table.current_mount;
		const char *drive_name = global_mount_table.mounts[mount_idx].mount_point;

		kprint("Dismounting ");
		kprint(drive_name);
		kprint("...\n");

		MountResult result = unmount_drive(&global_mount_table, mount_idx);
		if (result == MOUNT_SUCCESS) {
			kprint("Dismount successful!\n");
			
			/* Update prompt to reflect current drive or default */
			const char *new_prompt = get_current_prompt(&global_mount_table);
			input_set_prompt(&input, (char *)new_prompt);
		} else {
			kprint("Dismount failed: ");
			kprint(get_mount_result_string(result));
			kprint("\n");
		}
	}
	else if (strncmp_case_insensitive(subcmd, "format", subcmd_len) == 0 && subcmd_len == 6) {
		kprint("Formatting disk...\n");

		// Check if drive exists
		if (global_fs_map.drive_count == 0) {
			kprint("No drives available to format.\n");
			return;
		}
		
		char *device_args = skip_spaces(subcmd_end);
		if (device_args[0] != '\0')
		{
			kprint("  Device: ");
			kprint(device_args);
			kprint_newline();
		}

		char *fs_type_args = skip_spaces(find_space(device_args));
		if (fs_type_args[0] != '\0')
		{
			kprint("  Filesystem Type: ");
			kprint(fs_type_args);
			kprint_newline();
		}

		// Get position of drive based on device_args
		// For simplicity, assume device_args is "ide0", "ide1", etc
		int pos = -1;
		// Check for exact match by verifying next character is space/tab/null
		if (strncmp_case_insensitive(device_args, "ide0", 4) == 0 && 
		    (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 0;
		} else if (strncmp_case_insensitive(device_args, "ide1", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 1;
		} else if (strncmp_case_insensitive(device_args, "ide2", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 2;
		} else if (strncmp_case_insensitive(device_args, "ide3", 4) == 0 && 
		           (device_args[4] == '\0' || device_args[4] == ' ' || device_args[4] == '\t')) {
			pos = 3;
		} else {
			kprint("Unknown device specified.\n");
			return;
		}

		DriveInfo *drive = &global_fs_map.drives[pos];
		
		// Check if drive is actually present
		if (!drive->present) {
			kprint("Error: Drive ");
			kprint(device_args);
			kprint(" is not present.\n");
			return;
		}
		FormatOptions opts = {0};
		strcpy_local(opts.volume_label, "MYVOLUME");
		opts.quick_format = 1;

		FormatResult result = format_drive(drive, &opts);
		if (result == FORMAT_SUCCESS) {
			kprint("Format successful!\n");
		} else {
			kprint(get_format_result_string(result));
		}
	}
	else
	{
		kprint("Unknown disk command. Use 'list', 'mount', 'dismount', or 'format'.\n");
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

void cmd_switch(char *args)
{
	char *drive_id = skip_spaces(args);
	
	if (drive_id[0] == '\0')
	{
		kprint("Usage: switch <drive>\n");
		kprint("Example: switch ide0\n");
		return;
	}
	
	if (set_current_drive(&global_mount_table, drive_id))
	{
		kprint("Switched to ");
		kprint(drive_id);
		kprint("\n");
		
		/* Update prompt */
		const char *new_prompt = get_current_prompt(&global_mount_table);
		input_set_prompt(&input, (char *)new_prompt);
	}
	else
	{
		kprint("Drive not mounted or not found.\n");
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
	{"disk", (void *)cmd_disk, 1, 0, "Disk operations (list, mount, dismount, format)"},
	{"switch", (void *)cmd_switch, 1, 0, "Switch to mounted drive"},
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

/* Local string copy implementation */
static void strcpy_local(char *dest, const char *src)
{
	while (*src != '\0')
	{
		*dest++ = *src++;
	}
	*dest = '\0';
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
