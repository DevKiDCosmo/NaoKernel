/*
 * Input System Implementation
 * Handles keyboard input and line buffering
 */

#include "input.h"

/* External references */
extern unsigned int current_loc;
extern char *vidptr;
extern unsigned char keyboard_map[128];
extern void kprint(const char *str);
extern void kprint_newline(void);
extern void update_hardware_cursor(void);
extern void scroll_screen(void);

#define SCREENSIZE (2 * 80 * 25)

/* Global input buffer */
static InputBuffer global_input;
static CommandHistory global_history;

/* String utility functions */
int strcmp_custom(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp_custom(const char *s1, const char *s2, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
			return (unsigned char)s1[i] - (unsigned char)s2[i];
		}
	}
	return 0;
}

int strlen_custom(const char *str)
{
	int len = 0;
	while (str[len] != '\0') {
		len++;
	}
	return len;
}

void strcpy_custom(char *dest, const char *src)
{
	int i = 0;
	while (src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
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
	/* Store pointer to use */
	global_input = *inp;
	
	/* Print prompt */
	input_print_prompt(&global_input);
	
	/* Reset for new input */
	input_reset(&global_input);
	
	/* Wait for input to be ready */
	while (!global_input.ready) {
		/* Spin wait - keyboard handler will set ready flag */
	}
	
	/* Copy back to caller's buffer */
	*inp = global_input;
	
	/* Reset ready flag for next call */
	inp->ready = 0;
	
	return inp->buffer;
}

/* Add character to input buffer */
void input_add_char(InputBuffer *inp, char c)
{
	if (inp->position < MAX_INPUT_LENGTH - 1) {
		/* Check if we need to scroll before adding character */
		if (current_loc >= SCREENSIZE) {
			scroll_screen();
		}
		
		inp->buffer[inp->position++] = c;
		inp->buffer[inp->position] = '\0';
		
		/* Echo character to screen */
		vidptr[current_loc++] = c;
		vidptr[current_loc++] = 0x07;
		update_hardware_cursor();
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
			update_hardware_cursor();
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

/* Handle keyboard input */
void input_handle_keyboard(char keycode)
{
	/* Ignore key release events (keycode < 0 or >= 0x80) */
	if (keycode < 0 || keycode >= 0x80) {
		return;
	}
	
	/* Handle Enter key */
	if (keycode == ENTER_KEY_CODE) {
		input_complete(&global_input);
		return;
	}
	
	/* Handle Backspace key */
	if (keycode == BACKSPACE_KEY_CODE) {
		input_backspace(&global_input);
		return;
	}
	
	/* Handle regular character input */
	if (keycode < 128) {
		char ch = keyboard_map[(unsigned char)keycode];
		/* Only accept printable ASCII */
		if (ch >= 32 && ch <= 126) {
			input_add_char(&global_input, ch);
		}
	}
}

/* Initialize command history */
void history_init(CommandHistory *hist)
{
	int i, j;
	hist->count = 0;
	hist->current = -1;
	
	for (i = 0; i < MAX_HISTORY; i++) {
		for (j = 0; j < MAX_INPUT_LENGTH; j++) {
			hist->commands[i][j] = '\0';
		}
	}
}

/* Add command to history */
void history_add(CommandHistory *hist, const char *command)
{
	/* Don't add empty commands */
	if (command[0] == '\0') {
		return;
	}
	
	/* Shift older commands if history is full */
	if (hist->count >= MAX_HISTORY) {
		int i;
		for (i = 0; i < MAX_HISTORY - 1; i++) {
			strcpy_custom(hist->commands[i], hist->commands[i + 1]);
		}
		hist->count = MAX_HISTORY - 1;
	}
	
	/* Add new command to end */
	strcpy_custom(hist->commands[hist->count], command);
	hist->count++;
	hist->current = -1;  /* Reset position after adding new command */
}

/* Get previous command from history */
char* history_previous(CommandHistory *hist)
{
	if (hist->count == 0) {
		return 0;
	}
	
	if (hist->current == -1) {
		hist->current = hist->count - 1;
	} else if (hist->current > 0) {
		hist->current--;
	}
	
	return hist->commands[hist->current];
}

/* Get next command from history */
char* history_next(CommandHistory *hist)
{
	if (hist->count == 0) {
		return 0;
	}
	
	if (hist->current < hist->count - 1) {
		hist->current++;
		return hist->commands[hist->current];
	}
	
	hist->current = -1;
	return 0;
}

/* Reset history position */
void history_reset_position(CommandHistory *hist)
{
	hist->current = -1;
}
