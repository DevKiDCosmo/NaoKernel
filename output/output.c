/*
 * Output System Implementation
 * Screen output, cursor management, and formatting
 */

#include "output.h"

/* External video memory pointer and cursor location */
extern unsigned int current_loc;
extern char *vidptr;
extern void write_port(unsigned short port, unsigned char data);

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
			kprint_newline();
			i++;
		} else {
			vidptr[current_loc++] = str[i++];
			vidptr[current_loc++] = 0x07;
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
