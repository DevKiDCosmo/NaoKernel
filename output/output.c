/*
 * Output System Implementation
 * Screen output, cursor management, and formatting
 */

#include "output.h"

/* External video memory pointer and cursor location */
extern unsigned int current_loc;
extern char *vidptr;
extern void write_port(unsigned short port, unsigned char data);

/* Global output history */
static OutputHistory global_output_history;
static char current_line_buffer[MAX_LINE_LENGTH];
static int current_line_pos = 0;

/* Update hardware cursor position to match software cursor */
void update_hardware_cursor(void)
{
	/* Convert byte offset to character position (divide by 2) */
	unsigned short position = current_loc / 2;
	
	/* Send high byte to VGA */
	write_port(0x3D4, 14);
	write_port(0x3D5, (position >> 8) & 0xFF);
	
	/* Send low byte to VGA */
	write_port(0x3D4, 15);
	write_port(0x3D5, position & 0xFF);
}

/* Helper: copy string with length limit */
static void strncpy_safe(char *dest, const char *src, int max_len)
{
	int i = 0;
	while (i < max_len - 1 && src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
}

/* Initialize output history */
void output_history_init(OutputHistory *hist)
{
	int i, j;
	hist->count = 0;
	hist->scroll_offset = 0;
	for (i = 0; i < MAX_OUTPUT_LINES; i++) {
		for (j = 0; j < MAX_LINE_LENGTH; j++) {
			hist->lines[i][j] = '\0';
		}
	}
}

/* Add a line to output history */
void output_history_add_line(OutputHistory *hist, const char *line)
{
	if (line[0] == '\0') {
		return;
	}
	
	/* Shift older lines if history is full */
	if (hist->count >= MAX_OUTPUT_LINES) {
		int i;
		for (i = 0; i < MAX_OUTPUT_LINES - 1; i++) {
			strncpy_safe(hist->lines[i], hist->lines[i + 1], MAX_LINE_LENGTH);
		}
		hist->count = MAX_OUTPUT_LINES - 1;
	}
	
	/* Add new line to end */
	strncpy_safe(hist->lines[hist->count], line, MAX_LINE_LENGTH);
	hist->count++;
}

/* Scroll up in output history */
void output_history_scroll_up(OutputHistory *hist)
{
	int max_offset = hist->count - LINES + 1;
	if (max_offset < 0) max_offset = 0;
	
	if (hist->scroll_offset < max_offset) {
		hist->scroll_offset++;
	}
}

/* Scroll down in output history */
void output_history_scroll_down(OutputHistory *hist)
{
	if (hist->scroll_offset > 0) {
		hist->scroll_offset--;
	}
}

/* Display output history at current scroll offset */
void output_history_display(OutputHistory *hist)
{
	int i;
	int start_line;
	
	/* Clear screen first */
	clear_screen();
	
	/* Calculate starting line based on scroll offset */
	if (hist->count <= LINES - 1) {
		start_line = 0;
	} else {
		start_line = hist->count - (LINES - 1) - hist->scroll_offset;
		if (start_line < 0) start_line = 0;
	}
	
	/* Display lines */
	for (i = start_line; i < hist->count && i < start_line + LINES - 1; i++) {
		kprint(hist->lines[i]);
		kprint_newline();
	}
	
	/* Show scroll indicator if scrolled */
	if (hist->scroll_offset > 0) {
		kprint("--- Scrolled (Shift+PgDn to return) ---");
	}
}

/* Get output history pointer */
OutputHistory* get_output_history(void)
{
	return &global_output_history;
}

/* Add character to current line buffer */
static void add_to_line_buffer(char c)
{
	if (c == CHAR_NEWLINE) {
		/* Flush current line to history */
		current_line_buffer[current_line_pos] = '\0';
		output_history_add_line(&global_output_history, current_line_buffer);
		current_line_pos = 0;
		current_line_buffer[0] = '\0';
	} else if (current_line_pos < MAX_LINE_LENGTH - 1) {
		current_line_buffer[current_line_pos++] = c;
		current_line_buffer[current_line_pos] = '\0';
	}
}

/* Print a string to screen */
void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		/* Check if we need to scroll before printing */
		if (current_loc >= SCREENSIZE) {
			scroll_screen();
		}
		
		/* Check for newline character (ASCII 10) and handle line break */
		if (str[i] == CHAR_NEWLINE) {
			add_to_line_buffer(CHAR_NEWLINE);
			kprint_newline();
			i++;
		} else {
			add_to_line_buffer(str[i]);
			vidptr[current_loc++] = str[i++];
			vidptr[current_loc++] = 0x07;
		}
	}
	update_hardware_cursor();
}

/* Print a string with custom color */
void kprint_colored(const char *str, unsigned char color)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		/* Check if we need to scroll before printing */
		if (current_loc >= SCREENSIZE) {
			scroll_screen();
		}
		
		/* Check for newline character (ASCII 10) and handle line break */
		if (str[i] == CHAR_NEWLINE) {
			add_to_line_buffer(CHAR_NEWLINE);
			kprint_newline();
			i++;
		} else {
			add_to_line_buffer(str[i]);
			vidptr[current_loc++] = str[i++];
			vidptr[current_loc++] = color;
		}
	}
	update_hardware_cursor();
}

/* Scroll the screen up by one line */
void scroll_screen(void)
{
	unsigned int i;
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	
	/* Copy each line to the line above it */
	for (i = 0; i < (LINES - 1) * line_size; i++) {
		vidptr[i] = vidptr[i + line_size];
	}
	
	/* Clear the last line */
	for (i = (LINES - 1) * line_size; i < LINES * line_size; i += 2) {
		vidptr[i] = ' ';
		vidptr[i + 1] = 0x07;
	}
	
	/* Move cursor to start of last line */
	current_loc = (LINES - 1) * line_size;
	update_hardware_cursor();
}

/* Print a newline */
void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
	
	/* Check if we need to scroll */
	if (current_loc >= SCREENSIZE) {
		scroll_screen();
	}
	
	update_hardware_cursor();
}

/* Print a single character */
void kprint_char(char c)
{
	vidptr[current_loc++] = c;
	vidptr[current_loc++] = 0x07;
	update_hardware_cursor();
}

/* Print a number in hexadecimal */
void kprint_hex(unsigned int num)
{
	char hex_chars[] = "0123456789ABCDEF";
	char buffer[11];
	int i = 0;
	
	buffer[i++] = '0';
	buffer[i++] = 'x';
	
	if (num == 0) {
		buffer[i++] = '0';
	} else {
		int started = 0;
		int shift;
		for (shift = 28; shift >= 0; shift -= 4) {
			int digit = (num >> shift) & 0xF;
			if (digit != 0 || started) {
				buffer[i++] = hex_chars[digit];
				started = 1;
			}
		}
	}
	
	buffer[i] = '\0';
	kprint(buffer);
}

/* Print a number in decimal */
void kprint_dec(int num)
{
	char buffer[12];
	int i = 0;
	int is_negative = 0;
	
	if (num < 0) {
		is_negative = 1;
		num = -num;
	}
	
	if (num == 0) {
		buffer[i++] = '0';
	} else {
		int temp = num;
		int digits = 0;
		while (temp > 0) {
			temp /= 10;
			digits++;
		}
		
		i = digits;
		while (num > 0) {
			buffer[--i] = '0' + (num % 10);
			num /= 10;
		}
		i = digits;
	}
	
	buffer[i] = '\0';
	
	if (is_negative) {
		kprint_char('-');
	}
	kprint(buffer);
}

/* Clear the entire screen */
void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
	current_loc = 0;
	update_hardware_cursor();
	
	/* Initialize output history on first clear */
	static int history_initialized = 0;
	if (!history_initialized) {
		output_history_init(&global_output_history);
		current_line_pos = 0;
		current_line_buffer[0] = '\0';
		history_initialized = 1;
	}
}

/* Get current cursor position */
unsigned int get_cursor_position(void)
{
	return current_loc;
}

/* Set cursor position */
void set_cursor_position(unsigned int pos)
{
	if (pos < SCREENSIZE) {
		current_loc = pos;
		update_hardware_cursor();
	}
}
