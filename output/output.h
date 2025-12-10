/*
 * Output System - Screen output and formatting
 */

#ifndef OUTPUT_H
#define OUTPUT_H

/* Screen dimensions */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

/* Special characters - using ASCII values without raw chars */
#define CHAR_NEWLINE (10)

/* Output functions */
void kprint(const char *str);
void kprint_newline(void);
void kprint_char(char c);
void kprint_hex(unsigned int num);
void kprint_dec(int num);
void clear_screen(void);

/* Cursor management */
unsigned int get_cursor_position(void);
void set_cursor_position(unsigned int pos);
void update_hardware_cursor(void);

#endif /* OUTPUT_H */
