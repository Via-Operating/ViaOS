// MXOS -- Minimal X86 Operating System
// Kernel
// GNU GPL 3.0 License, Read LICENSE.TXT

#include "types.h"

/* Hardware text mode color constants. */
enum vga_color 
{
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	if (c == '\n')
	{
		y += 1;
		terminal_row = y;
		terminal_column = 0;
		return;
	}

	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);

	// terminal_row = y;
	// terminal_column = x;
}

void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void printf(const char* data) 
{
	terminal_write(data, strlen(data));
}

void init()
{
	terminal_initialize();

	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));

	printf(" [OK] Initialize Console\n");
	printf("[OK] Starting MXOS...\n");
}

#define IDT_SIZE 256 // only 8 bit interrupts? what the fuck?
#define KERNEL_CODE_SEGMENT_OFFSET 0x8
#define IDT_INTERRUPT_GATE_32BIT 0x8e
#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64 // hex minecraft stack of bits

#include "../drivers/intel/intel_ps2keyboard/keyboard_map.h"

extern void load_gdt();
extern void keyboard_handler();
extern char ioport_in(unsigned short port); // uint16_t port
extern void ioport_out(unsigned short port, unsigned char data);
extern void load_idt(unsigned int* idt_address);
extern void enable_interrupts();

struct IDT_pointer
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

struct IDT_entry
{
	unsigned short offset_lowerbits;
	unsigned short selector;
	unsigned char zero; // reserved
	unsigned char type_attr;
	unsigned short offset_upperbits;
} __attribute__((packed));

struct IDT_entry IDT[IDT_SIZE]; // entire IDT
int cursor_pos = 0;

void init_idt()
{
	unsigned int offset = (unsigned int)keyboard_handler;
	IDT[0x21].offset_lowerbits = offset & 0x0000FFFF;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0; // reserved
	IDT[0x21].type_attr = IDT_INTERRUPT_GATE_32BIT;
	IDT[0x21].offset_upperbits = (offset & 0xFFFF0000) >> 16;
	// ICW1
	ioport_out(PIC1_COMMAND_PORT, 0x11);
	ioport_out(PIC2_COMMAND_PORT, 0x11);
	// ICW2
	ioport_out(PIC1_DATA_PORT, 0x20);
	ioport_out(PIC1_DATA_PORT, 0x28);
	// ICW3
	ioport_out(PIC1_DATA_PORT, 0x0);
	ioport_out(PIC2_DATA_PORT, 0x0);
	// ICW4
	ioport_out(PIC1_DATA_PORT, 0x1);
	ioport_out(PIC1_DATA_PORT, 0x1);
	// Mask interrupts
	ioport_out(PIC1_DATA_PORT, 0xff);
	ioport_out(PIC1_DATA_PORT, 0xff);
	// Load IDT Data Structure
	struct IDT_pointer idt_ptr;
	idt_ptr.limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1; // i dont know wtf im doing
	idt_ptr.base = (unsigned int) &IDT;
	load_idt(&idt_ptr);

	printf("[OK] Initialized IDT\n");
}

void kb_init()
{
	ioport_out(PIC1_DATA_PORT, 0xFD);
	printf("[OK] Initialized Intel PS2 Keyboard\n");
}

void handle_keyboard_interrupt()
{
	ioport_out(PIC1_COMMAND_PORT, 0x20);
	unsigned char status = ioport_in(KEYBOARD_STATUS_PORT);
	if(status & 0x1)
	{
		char keycode = ioport_in(KEYBOARD_DATA_PORT);
		if(keycode < 0 || keycode >= 128) return;
		//terminal_putchar(keyboard_map[keycode]);
		terminal_putentryat(keyboard_map[keycode], terminal_color, cursor_pos, 10);
		cursor_pos++;
	}
}

void welcome()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

	printf("Welcome to ViaOS 1.0.0\n");
	printf("KERNEL VERSION 0.1\n");
	printf("Copyright 2024 Via\n");
}

void kmain()
{
	/* Initialize everything */
	init();
	init_idt();
	kb_init();
	welcome();
	enable_interrupts();

	while(1);
}