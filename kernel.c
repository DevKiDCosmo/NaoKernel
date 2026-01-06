/*
* Copyright (C) 2014  Arjun Sreedharan
* License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
*/
#include "keyboard_map.h"
#include "shell/shell.h"
#include "output/output.h"
#include "input/input.h"
#include "fs/fs.h"
#include "fs/mount/mount.h"
#include "fs/fileops.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
#define SIGUSR1 10

#define MOUNTED 0 // If 1 filesystem is mounted and file operations can be performed

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

/* Global filesystem map for shell access */
FilesystemMap global_fs_map;
/* Global mount table */
MountTable global_mount_table;

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


static const char* get_cpu_arch(void)
{
#if defined(__x86_64__) || defined(_M_X64)
	return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
	return "x86";
#elif defined(__aarch64__)
	return "aarch64";
#elif defined(__arm__) || defined(_M_ARM)
	return "arm";
#elif defined(__riscv)
	return "riscv";
#elif defined(__mips__)
	return "mips";
#elif defined(__powerpc64__)
	return "ppc64";
#elif defined(__powerpc__)
	return "ppc";
#elif defined(__sparc__)
	return "sparc";
#else
	return "unknown";
#endif
}

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
static void get_x86_vendor(char vendor[13])
{
	unsigned int eax, ebx, ecx, edx;
	eax = 0;
	/* cpuid with eax=0 returns vendor ID string in ebx, edx, ecx */
#if defined(__GNUC__)
	__asm__ __volatile__ (
		"cpuid"
		: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(0)
	);
#elif defined(_MSC_VER)
	int regs[4];
	__cpuid(regs, 0);
	ebx = (unsigned)regs[1];
	edx = (unsigned)regs[3];
	ecx = (unsigned)regs[2];
#else
	ebx = ecx = edx = 0;
#endif
	((unsigned int*)vendor)[0] = ebx;
	((unsigned int*)vendor)[1] = edx;
	((unsigned int*)vendor)[2] = ecx;
	vendor[12] = '\0';
}
#endif

void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		/* Delegate to shell keyboard handler */
		shell_handle_keyboard(keycode);
	}
}

void kmain(void)
{
	clear_screen();
	kprint("NaoKernel - Initializing...");
	kprint_newline();

	/* Basic CPU architecture report */
	kprint("CPU Architecture: ");
	kprint(get_cpu_arch());
	kprint_newline();

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
	char vendor[13];
	get_x86_vendor(vendor);
	kprint("CPU Vendor: ");
	kprint(vendor);
	kprint_newline();
#endif

	idt_init();
	kb_init();

	/* Initialize Ram and Drives */
	
	/* Initialize ramdisk filesystem */
	kprint("Initializing ramdisk filesystem...\n");
	fileops_init();

	/* Initialize filesystems */
    fs_init(&global_fs_map);
	
	/* Initialize mount table */
	mount_init(&global_mount_table);

	/* Initialize Service for Shell instead making the shell part of the kernel. */
	/* I need to think again.*/

	/* Start shell */
	nano_shell();

	while(1);
}