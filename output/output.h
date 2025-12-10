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

/* Output history configuration */
#define MAX_OUTPUT_LINES 500
#define MAX_LINE_LENGTH 256

/* Output history structure */
typedef struct {
	char lines[MAX_OUTPUT_LINES][MAX_LINE_LENGTH];
	int count;
	int scroll_offset;  /* Current scroll offset (0 = most recent) */
} OutputHistory;

/* Output functions */
void kprint(const char *str);
void kprint_newline(void);
void kprint_char(char c);
void kprint_hex(unsigned int num);
void kprint_dec(int num);
void kprint_colored(const char *str, unsigned char color);
void clear_screen(void);
void scroll_screen(void);

/* Cursor management */
unsigned int get_cursor_position(void);
void set_cursor_position(unsigned int pos);
void update_hardware_cursor(void);

/* Output history functions */
void output_history_init(OutputHistory *hist);
void output_history_add_line(OutputHistory *hist, const char *line);
void output_history_scroll_up(OutputHistory *hist);
void output_history_scroll_down(OutputHistory *hist);
void output_history_display(OutputHistory *hist);
OutputHistory* get_output_history(void);

#endif /* OUTPUT_H */
