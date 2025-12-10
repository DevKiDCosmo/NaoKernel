/*
 * NaoKernel Nano Shell
 * Minimal shell with echo and exit commands
 */

/* Screen dimensions */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define MAX_INPUT_LENGTH 256

/* Global state */
extern unsigned int current_loc;
extern char *vidptr;
extern unsigned char keyboard_map[128];

/* Input buffer structure */
typedef struct {
	char buffer[MAX_INPUT_LENGTH];
	int position;
	int ready;
	char *prompt;
} InputBuffer;

/* Shell state */
InputBuffer input;
int shell_running = 1;

/* Forward declarations */
void kprint(const char *str);
void kprint_newline(void);

/* Compare two strings */
int strcmp_custom(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return (unsigned char)*s1 - (unsigned char)*s2;
}

/* String length */
int strlen_custom(const char *str)
{
	int len = 0;
	while (str[len] != '\0') {
		len++;
	}
	return len;
}

/* Initialize input buffer */
void input_init(InputBuffer *inp, char *prompt)
{
	inp->position = 0;
	inp->buffer[0] = '\0';
	inp->ready = 0;
	inp->prompt = prompt;
}

/* Reset input buffer for new input */
void input_reset(InputBuffer *inp)
{
	inp->position = 0;
	inp->buffer[0] = '\0';
	inp->ready = 0;
}

/* Print prompt */
void input_print_prompt(InputBuffer *inp)
{
	if (inp->prompt) {
		kprint(inp->prompt);
	}
}

/* Get input line (blocking) - waits for Enter key */
char* input_getline(InputBuffer *inp)
{
	/* Print prompt */
	input_print_prompt(inp);
	
	/* Reset for new input */
	input_reset(inp);
	
	/* Wait for input to be ready */
	while (!inp->ready) {
		/* Spin wait - keyboard handler will set ready flag */
	}
	
	/* Reset ready flag for next call */
	inp->ready = 0;
	
	return inp->buffer;
}

/* Add character to input buffer */
void input_add_char(InputBuffer *inp, char c)
{
	if (inp->position < MAX_INPUT_LENGTH - 1) {
		inp->buffer[inp->position++] = c;
		inp->buffer[inp->position] = '\0';
		
		/* Echo character to screen */
		vidptr[current_loc++] = c;
		vidptr[current_loc++] = 0x07;
	}
}

/* Handle backspace in input buffer */
void input_backspace(InputBuffer *inp)
{
	if (inp->position > 0) {
		inp->position--;
		inp->buffer[inp->position] = '\0';
		
		/* Move cursor back and clear character */
		if (current_loc >= 2) {
			current_loc -= 2;
			vidptr[current_loc] = ' ';
			vidptr[current_loc + 1] = 0x07;
		}
	}
}

/* Mark input as complete */
void input_complete(InputBuffer *inp)
{
	inp->buffer[inp->position] = '\0';
	inp->ready = 1;
	kprint_newline();
}

/* Echo command implementation */
void cmd_echo(char *args)
{
	if (args[0] != '\0') {
		kprint(args);
	}
	kprint_newline();
}

/* Parse and execute shell commands */
void shell_execute_command(char *command)
{
	/* Skip leading spaces */
	int i = 0;
	while (command[i] == ' ') i++;
	
	/* Check for exit command */
	if (strcmp_custom(&command[i], "exit") == 0) {
		kprint("Shutting down...\n");
		shell_running = 0;
		return;
	}
	
	/* Check for echo command */
	if (strcmp_custom(&command[i], "echo") == 0) {
		i += 4; /* skip "echo" */
		while (command[i] == ' ') i++;
		cmd_echo(&command[i]);
		return;
	}
	
	/* Unknown command */
	if (command[i] != '\0') {
		kprint("Unknown command: ");
		kprint(&command[i]);
		kprint_newline();
	}
}

/* Nano shell main loop */
void nano_shell(void)
{
	char *line;
	
	kprint("\n=== NaoKernel Nano Shell ===\n");
	kprint("Commands: echo, exit\n\n");
	
	/* Initialize input system with prompt */
	input_init(&input, "> ");
	
	while (shell_running) {
		/* Get line of input (blocks until Enter is pressed) */
		line = input_getline(&input);
		
		/* Execute command if not empty */
		if (strlen_custom(line) > 0) {
			shell_execute_command(line);
		}
	}
}

/* Handle keyboard input for shell */
void shell_handle_keyboard(char keycode)
{
	/* Ignore key release events (keycode >= 0x80) */
	if (keycode < 0) {
		return;
	}
	
	/* Handle Enter key */
	if (keycode == ENTER_KEY_CODE) {
		input_complete(&input);
		return;
	}
	
	/* Handle Backspace key */
	if (keycode == BACKSPACE_KEY_CODE) {
		input_backspace(&input);
		return;
	}
	
	/* Handle regular character input */
	/* Only process valid scan codes */
	if (keycode < 128) {
		char ch = keyboard_map[(unsigned char) keycode];
		/* Only accept printable ASCII: space (32) through tilde (126) */
		/* This filters out control chars, null, tabs, etc. */
		if (ch >= 32 && ch <= 126) {
			input_add_char(&input, ch);
		}
	}
}
