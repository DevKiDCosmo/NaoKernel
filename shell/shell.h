/*
 * NaoKernel Shell Interface
 * Command-line interface with multiple commands
 */

#ifndef SHELL_H
#define SHELL_H

/* Main shell function */
void nano_shell(void);

/* Keyboard handler - called from kernel interrupt handler */
void shell_handle_keyboard(char keycode);

#endif /* SHELL_H */
