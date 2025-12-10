/*
 * Input System Implementation
 * Handles keyboard input and line buffering
 */

#include "input.h"
#include "../output/output.h"

/* External references */
extern unsigned int current_loc;
extern char *vidptr;
extern unsigned char keyboard_map[128];
extern unsigned char keyboard_map_shifted[128];
extern void kprint(const char *str);
extern void kprint_newline(void);
extern void update_hardware_cursor(void);
extern void scroll_screen(void);

/* Global input buffer */
static InputBuffer global_input;
static CommandHistory global_history;

/* Keyboard state */
static int shift_pressed = 0;
static int caps_lock_on = 0;
static int escape_state = 0;  /* Track if we're in escape sequence */

/* Check if shift is currently pressed by reading keyboard controller */
static int is_shift_pressed(void)
{                                          
	return shift_pressed;
}

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

/* Custom memcpy implementation */
void* memcpy_custom(void *dest, const void *src, int n)
{
	int i;
	char *d = (char*)dest;
	const char *s = (const char*)src;
	
	for (i = 0; i < n; i++) {
		d[i] = s[i];
	}
	
	return dest;
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

/* Set the command history for input system */
void input_set_history(CommandHistory *hist)
{
	int i, j;
	
	/* Copy history data field by field to avoid memcpy */
	global_history.count = hist->count;
	global_history.current = hist->current;
	
	for (i = 0; i < MAX_HISTORY; i++) {
		global_history.valid[i] = hist->valid[i];
		for (j = 0; j < MAX_INPUT_LENGTH; j++) {
			global_history.commands[i][j] = hist->commands[i][j];
		}
	}
}

/* Get the updated history back */
CommandHistory* input_get_history(void)
{
	return &global_history;
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

/* Clear input buffer and redraw from history */
static void input_load_from_history(InputBuffer *inp, const char *history_cmd)
{
	int i;
	/* Clear current input by backspacing */
	while (inp->position > 0) {
		input_backspace(inp);
	}
	
	/* Load history command into buffer */
	if (history_cmd) {
		for (i = 0; history_cmd[i] != '\0' && i < MAX_INPUT_LENGTH - 1; i++) {
			input_add_char(inp, history_cmd[i]);
		}
	}
}

/* Scroll through history by pages */
static void input_scroll_history(CommandHistory *hist, int direction)
{
	int step = 5;  /* Scroll 5 commands at a time */
	int i;
	
	if (hist->count == 0) {
		return;
	}
	
	if (direction > 0) {
		/* Page Up - go backwards in history */
		for (i = 0; i < step; i++) {
			if (hist->current == -1) {
				hist->current = hist->count - 1;
			} else if (hist->current > 0) {
				hist->current--;
			} else {
				break;
			}
		}
	} else {
		/* Page Down - go forwards in history */
		for (i = 0; i < step; i++) {
			if (hist->current < hist->count - 1) {
				hist->current++;
			} else {
				hist->current = -1;
				break;
			}
		}
	}
}

/* Get character from keyboard map with modifiers applied */
static char get_char_with_modifiers(unsigned char keycode)
{
	char ch;
	int use_shifted;
	
	/* Check current shift state */
	use_shifted = is_shift_pressed();
	
	/* Get character from appropriate map */
	if (use_shifted) {
		ch = keyboard_map_shifted[keycode];
	} else {
		ch = keyboard_map[keycode];
	}
	
	/* Apply caps lock to letters only */
	if (caps_lock_on && ch >= 'a' && ch <= 'z') {
		ch = ch - 'a' + 'A';  /* Convert to uppercase */
	} else if (caps_lock_on && ch >= 'A' && ch <= 'Z') {
		ch = ch - 'A' + 'a';  /* Convert to lowercase if shift is also pressed */
	}
	
	return ch;
}

/* Handle keyboard input */
void input_handle_keyboard(char keycode)
{
	unsigned char ukey = (unsigned char)keycode;
	char *history_cmd;
	char ch;
	
	/* Handle escape byte (0xE0) for extended keys */
	if (ukey == 0xE0) {
		escape_state = 1;
		return;
	}
	
	/* Handle key release events (bit 7 set for standard keys) */
	if (ukey >= 0x80) {
		unsigned char release_code = ukey & 0x7F;
		
		/* Handle shift key release */
		if (release_code == LEFT_SHIFT_KEY_CODE || release_code == RIGHT_SHIFT_KEY_CODE) {
			shift_pressed = 0;
		}
		return;
	}
	
	/* Handle shift key press */
	if (keycode == LEFT_SHIFT_KEY_CODE || keycode == RIGHT_SHIFT_KEY_CODE) {
		shift_pressed = 1;
		return;
	}
	
	/* Handle caps lock toggle */
	if (keycode == CAPS_LOCK_KEY_CODE) {
		caps_lock_on = !caps_lock_on;
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
	
	/* Handle Up Arrow - previous command in history */
	if (keycode == UP_ARROW_KEY_CODE) {
		history_cmd = history_previous(&global_history);
		if (history_cmd) {
			input_load_from_history(&global_input, history_cmd);
		}
		return;
	}
	
	/* Handle Down Arrow - next command in history */
	if (keycode == DOWN_ARROW_KEY_CODE) {
		history_cmd = history_next(&global_history);
		if (history_cmd) {
			input_load_from_history(&global_input, history_cmd);
		} else {
			/* Clear input if at the end of history */
			input_load_from_history(&global_input, 0);
		}
		return;
	}
	
	/* Handle Page Up - jump to first (oldest) command */
	if (keycode == PAGE_UP_KEY_CODE) {
		if (global_history.count > 0) {
			global_history.current = 0;
			history_cmd = global_history.commands[0];
			input_load_from_history(&global_input, history_cmd);
		}
		return;
	}
	
	/* Handle Page Down - clear input field */
	if (keycode == PAGE_DOWN_KEY_CODE) {
		global_history.current = -1;
		input_load_from_history(&global_input, 0);
		return;
	}
	
	/* Handle regular character input */
	if (keycode < 128) {
		ch = get_char_with_modifiers((unsigned char)keycode);
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
		hist->valid[i] = 1;
		for (j = 0; j < MAX_INPUT_LENGTH; j++) {
			hist->commands[i][j] = '\0';
		}
	}
}

/* Add command to history */
void history_add(CommandHistory *hist, const char *command, int is_valid)
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
			hist->valid[i] = hist->valid[i + 1];
		}
		hist->count = MAX_HISTORY - 1;
	}
	
	/* Add new command to end */
	strcpy_custom(hist->commands[hist->count], command);
	hist->valid[hist->count] = is_valid;
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
		/* Starting from current input, go to most recent history */
		hist->current = hist->count - 1;
	} else if (hist->current > 0) {
		/* Go to older command */
		hist->current--;
	} else if (hist->current == 0) {
		/* Already at oldest, stay there */
		return hist->commands[0];
	}
	
	return hist->commands[hist->current];
}

/* Get next command from history */
char* history_next(CommandHistory *hist)
{
	if (hist->count == 0) {
		return 0;
	}
	
	if (hist->current == -1) {
		/* Already at the end (current input), stay there */
		return 0;
	}
	
	if (hist->current < hist->count - 1) {
		/* Go to newer command */
		hist->current++;
		return hist->commands[hist->current];
	} else {
		/* At most recent history item, go back to current input */
		hist->current = -1;
		return 0;  /* Return null to clear input */
	}
	
	/* Reached the newest command, clear input */
	hist->current = -1;
	return 0;
}

/* Reset history position */
void history_reset_position(CommandHistory *hist)
{
	hist->current = -1;
}
