/*
 * NaoKernel Shell Interface
 * Reusable input/output functions for shell and other components
 */

#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT_LENGTH 256

/* Input buffer structure for reusable line input */
typedef struct {
	char buffer[MAX_INPUT_LENGTH];
	int position;
	int ready;
	char *prompt;
} InputBuffer;

/* String utility functions */
int strcmp_custom(const char *s1, const char *s2);
int strlen_custom(const char *str);

/* Input system functions */
void input_init(InputBuffer *inp, char *prompt);
void input_reset(InputBuffer *inp);
void input_print_prompt(InputBuffer *inp);
char* input_getline(InputBuffer *inp);
void input_add_char(InputBuffer *inp, char c);
void input_backspace(InputBuffer *inp);
void input_complete(InputBuffer *inp);

/* Main shell function */
void nano_shell(void);

/* Keyboard handler - called from kernel interrupt handler */
void shell_handle_keyboard(char keycode);

#endif /* SHELL_H */
