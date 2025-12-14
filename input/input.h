/*
 * Input System - Keyboard and input handling
 */

#ifndef INPUT_H
#define INPUT_H

#define MAX_INPUT_LENGTH 256
#define MAX_HISTORY 100  /* Maximum number of commands to remember */
#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define UP_ARROW_KEY_CODE 0x48
#define DOWN_ARROW_KEY_CODE 0x50
#define LEFT_SHIFT_KEY_CODE 0x2A
#define RIGHT_SHIFT_KEY_CODE 0x36
#define CAPS_LOCK_KEY_CODE 0x3A
#define PAGE_UP_KEY_CODE 0x49
#define PAGE_DOWN_KEY_CODE 0x51

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
	int valid[MAX_HISTORY];  /* 1 if command was valid, 0 if invalid */
	int count;
	int current;  /* Current position in history (-1 = no history selected) */
} CommandHistory;

/* String utility functions */
int strcmp_custom(const char *s1, const char *s2);
static int strlen_custom(const char *str);
void strcpy_custom(char *dest, const char *src);
int strncmp_custom(const char *s1, const char *s2, int n);
void* memcpy_custom(void *dest, const void *src, int n);

/* Input system functions */
void input_init(InputBuffer *inp, char *prompt);
void input_reset(InputBuffer *inp);
void input_print_prompt(InputBuffer *inp);
void input_set_prompt(InputBuffer *inp, char *prompt);
char* input_getline(InputBuffer *inp);
void input_add_char(InputBuffer *inp, char c);
void input_backspace(InputBuffer *inp);
void input_complete(InputBuffer *inp);
void input_set_history(CommandHistory *hist);
CommandHistory* input_get_history(void);

/* History functions */
void history_init(CommandHistory *hist);
void history_add(CommandHistory *hist, const char *command, int is_valid);
char* history_previous(CommandHistory *hist);
char* history_next(CommandHistory *hist);
void history_reset_position(CommandHistory *hist);

/* Keyboard handler - called from kernel interrupt handler */
void input_handle_keyboard(char keycode);

#endif /* INPUT_H */
