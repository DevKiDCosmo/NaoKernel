/*
 * NaoKernel Shell
 * Command-line interface with multiple commands
 */

#include "../input/input.h"
#include "../output/output.h"

/* Shell state */
static InputBuffer input;
static int shell_running = 1;

/* Command implementations */

/* Help command - show available commands */
void cmd_help(void)
{
	kprint("Available commands:\n");
	kprint("  help    - Show this help message\n");
	kprint("  clear   - Clear the screen\n");
	kprint("  echo    - Echo text to screen\n");
	kprint("  about   - Show system information\n");
	kprint("  exit    - Shutdown the system\n");
	kprint("  test    - Run a test command\n");
}

/* Clear command - clear the screen */
void cmd_clear(void)
{
	clear_screen();
}

/* Echo command - print arguments */
void cmd_echo(char *args)
{
	if (args[0] != '\0') {
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

/* Skip leading whitespace */
static char* skip_spaces(char *str)
{
	while (*str == ' ' || *str == '\t') {
		str++;
	}
	return str;
}

/* Parse and execute shell commands */
void shell_execute_command(char *command)
{
	char *cmd;
	
	/* Skip leading spaces */
	cmd = skip_spaces(command);
	
	/* Empty command */
	if (cmd[0] == '\0') {
		return;
	}
	
	/* Help command */
	if (strcmp_custom(cmd, "help") == 0) {
		cmd_help();
		return;
	}
	
	/* Clear command */
	if (strcmp_custom(cmd, "clear") == 0) {
		cmd_clear();
		return;
	}
	
	/* About command */
	if (strcmp_custom(cmd, "about") == 0) {
		cmd_about();
		return;
	}
	
	/* Exit command */
	if (strcmp_custom(cmd, "exit") == 0) {
		cmd_exit();
		return;
	}

	if (strcmp_custom(cmd, "test") == 0) {
		cmd_test();
		return;
	}
	
	/* Echo command */
	if (strncmp_custom(cmd, "echo", 4) == 0) {
		char *args = cmd + 4;
		args = skip_spaces(args);
		cmd_echo(args);
		return;
	}
	
	/* Unknown command */
	kprint("Unknown command: ");
	kprint(cmd);
	kprint_newline();
	kprint("Type 'help' for available commands.\n");
}

/* Shell main loop */
void nano_shell(void)
{
	char *line;
	
	kprint("\n=== NaoKernel Shell ===\n");
	kprint("Type 'help' for available commands.\n\n");
	
	/* Initialize input system with prompt */
	input_init(&input, "> ");
	
	while (shell_running) {
		/* Get line of input (blocks until Enter is pressed) */
		line = input_getline(&input);
		
		/* Execute command */
		shell_execute_command(line);
	}
}

/* Handle keyboard input for shell */
void shell_handle_keyboard(char keycode)
{
	/* Delegate to input system */
	input_handle_keyboard(keycode);
}
