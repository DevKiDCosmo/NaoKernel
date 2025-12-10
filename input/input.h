/*
 * Input System - Keyboard and input handling
 */

#ifndef INPUT_H
#define INPUT_H

#define MAX_INPUT_LENGTH 256
#define MAX_HISTORY 20  /* Maximum number of commands to remember */
#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define UP_ARROW_KEY_CODE 0x48
#define DOWN_ARROW_KEY_CODE 0x50

/* Input buffer structure for line input */
typedef struct {
	char buffer[MAX_INPUT_LENGTH];
	int position;
	int ready;
	char *prompt;
} InputBuffer;

/* Command history structure */
typedef struct {
	char commands[MAX_HISTORY][MAX_INPUT_LENGTH];
	int count;
	int current;  /* Current position in history (-1 = no history selected) */
} CommandHistory;

/* String utility functions */
int strcmp_custom(const char *s1, const char *s2);
static int strlen_custom(const char *str);
void strcpy_custom(char *dest, const char *src);
int strncmp_custom(const char *s1, const char *s2, int n);

/* Input system functions */
void input_init(InputBuffer *inp, char *prompt);
void input_reset(InputBuffer *inp);
void input_print_prompt(InputBuffer *inp);
char* input_getline(InputBuffer *inp);
void input_add_char(InputBuffer *inp, char c);
void input_backspace(InputBuffer *inp);
void input_complete(InputBuffer *inp);

/* History functions */
void history_init(CommandHistory *hist);
void history_add(CommandHistory *hist, const char *command);
char* history_previous(CommandHistory *hist);
char* history_next(CommandHistory *hist);
void history_reset_position(CommandHistory *hist);

/* Keyboard handler - called from kernel interrupt handler */
void input_handle_keyboard(char keycode);

#endif /* INPUT_H */
