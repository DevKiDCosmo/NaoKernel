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

/* Filesystem command declarations */
void cmd_ls(char *args);
void cmd_cat(char *args);
void cmd_touch(char *args);
void cmd_mkdir(char *args);
void cmd_rm(char *args);
void cmd_cp(char *args);
void cmd_pwd(void);
void cmd_cd(char *args);
void cmd_echo_to_file(char *args);

#endif /* SHELL_H */
