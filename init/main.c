// ViaOS
// Kernel
// GNU GPL 3.0 License, Read LICENSE.TXT

#include <via/stdio.h>
#include <via/shell/term/bash/echo.h>
#include <via/shell/term/bash/help.h>
#include <via/shell/term/bash/hi.h>
#include <via/shell/term/bash/ld.h>
#include <via/shell/term/bash/shutdown.h>
#include <via/via.h>

enum BOOL_T poweroff_sys = FALSE;

enum BOOL_T strncmp(const char *str1, const char *str2, size_t n) 
{
    // Compare up to n characters or until a null terminator is encountered
    while (n > 0 && *str1 && *str2) {
        if (*str1 != *str2) {
        	if((unsigned char)*str1 - (unsigned char)*str2)
        	{
        		return TRUE;
        	}
        	else
        	{
        		return FALSE;
        	}
        }
        str1++;
        str2++;
        n--;
    }

    // If we've compared all n characters, or reached a null terminator
    if (n == 0) {
        return FALSE; // Strings are equal up to n characters
    }
    // If we reach here, it means n > 0 and either of the strings has ended
    if((unsigned char)*str1 - (unsigned char)*str2)
    {
		return TRUE;
    }
	else
	{
		return FALSE;
	}
}

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

void set_cursor_position(size_t row, size_t column);

void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}

	set_cursor_position(terminal_row, terminal_column);
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
}

#define IDT_SIZE 256 // only 8 bit of interrupts? what the fuck?
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
extern void disable_interrupts();

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

#define VGA_CONTROL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

// Define indices for the VGA registers
#define CURSOR_HIGH_BYTE 0x0E
#define CURSOR_LOW_BYTE 0x0F

void set_cursor_position(size_t row, size_t column) 
{
    // Calculate the cursor position in VGA memory
    uint16_t position = row * VGA_WIDTH + column;
    
    // Send the high byte of the cursor position to the VGA control port
    ioport_out(VGA_CONTROL_PORT, CURSOR_HIGH_BYTE);
    ioport_out(VGA_DATA_PORT, (uint8_t)(position >> 8)); // High byte

    // Send the low byte of the cursor position to the VGA control port
    ioport_out(VGA_CONTROL_PORT, CURSOR_LOW_BYTE);
    ioport_out(VGA_DATA_PORT, (uint8_t)(position & 0xFF)); // Low byte
}


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

void whoami()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
	printf("\nadmin\n");

	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
	printf("> ");
}

#define BUFFER_SIZE 128

static char input_buffer[BUFFER_SIZE];
static size_t buffer_pos = 0;

void handle_keyboard_interrupt()
{
	if(poweroff_sys == TRUE)
	{
		return;
	}

    ioport_out(PIC1_COMMAND_PORT, 0x20);
    unsigned char status = ioport_in(KEYBOARD_STATUS_PORT);

    if (status & 0x1)
    {
        char keycode = ioport_in(KEYBOARD_DATA_PORT);

        // Check for valid keycode
        if (keycode < 0 || keycode >= 128) return;

        // Map the keycode to a character
        char ch = keyboard_map[keycode];
        
        // Handle backspace
        if (ch == 0x08) // Backspace keycode
        {
            if (buffer_pos > 0)
            {
                buffer_pos--;
                terminal_putchar('\b'); // Display backspace
                terminal_putchar(' '); // Clear the last character
                terminal_putchar('\b'); // Move cursor back
            }
            return;
        }

        // Append character to buffer
        if (buffer_pos < BUFFER_SIZE - 1 && ch != '\0')
        {
            input_buffer[buffer_pos++] = ch;
            input_buffer[buffer_pos] = '\0'; // Null-terminate the string

            // Print character to terminal
            terminal_putchar(ch);

            if(ch == '\n')
            {
            	printf("> ");
            }

            // Commandline
            if (buffer_pos >= 3 &&
            	input_buffer[buffer_pos - 3] == 'h' &&
            	input_buffer[buffer_pos - 2] == 'i' &&
            	input_buffer[buffer_pos - 1] == '\n')
            {
                hello();
                buffer_pos -= 2; // Adjust buffer position to account for replacement
            }

            if (buffer_pos >= 5 &&
            	input_buffer[buffer_pos - 5] == 'h' &&
            	input_buffer[buffer_pos - 4] == 'e' &&
            	input_buffer[buffer_pos - 3] == 'l' &&
            	input_buffer[buffer_pos - 2] == 'p' &&
            	input_buffer[buffer_pos - 1] == '\n')
            {
                help_menu();
                buffer_pos -= 2; // Adjust buffer position to account for replacement
            }

            if (buffer_pos >= 3 &&
            	input_buffer[buffer_pos - 3] == 'l' &&
            	input_buffer[buffer_pos - 2] == 'd' &&
            	input_buffer[buffer_pos - 1] == '\n')
            {
                list_dir();
                buffer_pos -= 2; // Adjust buffer position to account for replacement
            }

		if (buffer_pos >= 8 &&
            	input_buffer[buffer_pos - 8] == 'c' &&
            	input_buffer[buffer_pos - 7] == 'r' &&
            	input_buffer[buffer_pos - 6] == 'e' &&
            	input_buffer[buffer_pos - 5] == 'd' &&
            	input_buffer[buffer_pos - 4] == 'i' &&
            	input_buffer[buffer_pos - 3] == 't' &&
            	input_buffer[buffer_pos - 2] == 's' &&
            	input_buffer[buffer_pos - 1] == '\n'
            {
                
		terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));


		printf("\n\n");
		printf(" Via Operating System\n");
		printf("--------------------\n\n");

		printf(" Main Developer/Founder : Bradinator\n");
		printf(" Co-Founder             : Kap Petrov\n");
		printf(" Developers             : ALocalDeveloper, Vincent392, vrified-stupd\n");

		buffer_pos -= 7;

            }
		
            if (buffer_pos >= 7 &&
            	input_buffer[buffer_pos - 7] == 'w' &&
            	input_buffer[buffer_pos - 6] == 'h' &&
            	input_buffer[buffer_pos - 5] == 'o' &&
            	input_buffer[buffer_pos - 4] == 'a' &&
            	input_buffer[buffer_pos - 3] == 'm' &&
            	input_buffer[buffer_pos - 2] == 'i' &&
            	input_buffer[buffer_pos - 1] == '\n')
            {
                whoami();
                buffer_pos -= 2; // Adjust buffer position to account for replacement
            }

            if (buffer_pos >= 9 &&
            	input_buffer[buffer_pos - 9] == 's' &&
            	input_buffer[buffer_pos - 8] == 'h' &&
            	input_buffer[buffer_pos - 7] == 'u' &&
            	input_buffer[buffer_pos - 6] == 't' &&
            	input_buffer[buffer_pos - 5] == 'd' &&
            	input_buffer[buffer_pos - 4] == 'o' &&
            	input_buffer[buffer_pos - 3] == 'w' &&
            	input_buffer[buffer_pos - 2] == 'n' &&
            	input_buffer[buffer_pos - 1] == '\n')
            {
                shutdown();
                disable_interrupts();
                poweroff_sys = TRUE;
                buffer_pos -= 2; // Adjust buffer position to account for replacement
                return;
            }
        }
    }
}


void welcome()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK))
	printf("[OK] Starting VDK...\n");

	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

	printf("Welcome to ViaOS 1.0.0\n");
	printf("KERNEL VERSION 0.5\n");
	printf("Copyright 2024 Via\n");
	printf("> ");
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
