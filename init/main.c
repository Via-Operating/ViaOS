// ViaOS
// VDK
// GNU GPL 3.0 License, Read LICENSE.TXT

#include <via/stdio.h>
#include <via/shell/term/viaSh/echo.h>
#include <via/shell/term/viaSh/help.h>
#include <via/shell/term/viaSh/hi.h>
#include <via/shell/term/viaSh/ld.h>
#include <via/shell/term/viaSh/cd.h>
#include <via/shell/term/viaSh/info.h>
#include <via/shell/term/viaSh/shutdown.h>
#include <via/shell/shell32/desktop/window.h>
#include <via/shell/shell32/desktop/taskbar.h>
#include <via/shell/shell32/accessories/notepad.h>
#include <via/shell/shell32/accessories/viaver.h>
#include <via/shell/shell32/accessories/paint.h>
#include <via/via.h>

#define MAX_FILENAME 255
#define MAX_FILES 255

struct RIFS_F all_files[MAX_FILES] = {0};

int strcpy(char *dst, const char *src) {
    int i = 0;
    while ((*dst++ = *src++) != 0)
        i++;
    return i;
}

enum BOOL_T poweroff_sys = FALSE;
enum BOOL_T isInstalled = TRUE;
enum BOOL_T continueSetup = FALSE;
enum BOOL_T isInput = TRUE;
enum BOOL_T stop1 = FALSE;
enum BOOL_T stop2 = FALSE;
enum BOOL_T serviceAgreed = FALSE;
enum BOOL_T servicePage = FALSE;
enum BOOL_T stop3 = FALSE;
enum BOOL_T stop4 = FALSE;
enum BOOL_T next = FALSE;
enum BOOL_T stop5 = FALSE;
enum BOOL_T stop6 = FALSE;
enum BOOL_T agreePage = FALSE;
enum BOOL_T stop7 = FALSE;
enum BOOL_T agreePage2 = FALSE;
enum BOOL_T kbI = FALSE;

int pageNumber = 0;
int xBitMap = 0;

uint16_t mx = 0;
uint16_t my = 0;

// TODO: Remove terminal functions, it's no longer needed. It's fully VGA now, although you can probably port these to VGA
// graphics mode.

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

void terminal_scroll(void) 
{
    // Move all rows up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current_index = y * VGA_WIDTH + x;
            const size_t next_index = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[current_index] = terminal_buffer[next_index];
        }
    }

    // Clear the last line
    const size_t last_row = VGA_HEIGHT - 1;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = last_row * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) 
{
    if (c == '\n') {
        terminal_row++;
        terminal_column = 0;
        if (terminal_row >= VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1; // keep at bottom row
            terminal_scroll();
        }
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row >= VGA_HEIGHT) {
                terminal_row = VGA_HEIGHT - 1; // keep at bottom row
                terminal_scroll();
            }
        }
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

	printf("[OK] Initialize Console\n");
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
#include "../drivers/independent/disk/ata/IDE.h"
#include "../drivers/independent/graphics/vga/vga.h"
//#include "../drivers/intel/intel_ps2mouse/mouse.h"

extern void load_gdt();
extern void keyboard_handler();
extern void ata_handler();
extern void m_handler();
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

void idt_set_entry(int index, uint32_t base, uint8_t flags)
{
    IDT[index].offset_lowerbits = base & 0x0000FFFF;
    IDT[index].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[index].zero = 0; // reserved
    IDT[index].type_attr = flags | 0x60;
    IDT[index].offset_upperbits = (base & 0xFFFF0000) >> 16;
}

void init_idt()
{
	// unsigned int offset = (unsigned int)keyboard_handler;
	// IDT[0x21].offset_lowerbits = offset & 0x0000FFFF;
	// IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	// IDT[0x21].zero = 0; // reserved
	// IDT[0x21].type_attr = IDT_INTERRUPT_GATE_32BIT;
	// IDT[0x21].offset_upperbits = (offset & 0xFFFF0000) >> 16;

    idt_set_entry(33, (unsigned int)keyboard_handler, IDT_INTERRUPT_GATE_32BIT);
    idt_set_entry(46, (unsigned int)ata_handler, IDT_INTERRUPT_GATE_32BIT);
    //idt_set_entry(44, (unsigned int)m_handler, IDT_INTERRUPT_GATE_32BIT);

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

static char username[BUFFER_SIZE];
static int username_pos = 0;

static char ageUser[BUFFER_SIZE];
static int ageUser_pos = 0;

void handle_keyboard_interrupt()
{
	kbI = TRUE;

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
            //terminal_putchar(ch);

            if(agreePage2 == TRUE && isInstalled == FALSE)
            {
                  pageNumber = 2;
            }
            else
            {
                bitmap_putchar(ch, BLACK);
            }

            if(ch == '\n')
            {
            	//printf("> ");
            }

            // Commandline
            if(isInstalled == TRUE)
            {
                if (buffer_pos >= 3 &&
                	input_buffer[buffer_pos - 3] == 'h' &&
                	input_buffer[buffer_pos - 2] == 'i' &&
                	input_buffer[buffer_pos - 1] == '\n')
                {
                    hello();
                    buffer_pos -= 2; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 5 &&
                    input_buffer[buffer_pos - 5] == 'i' &&
                    input_buffer[buffer_pos - 4] == 'n' &&
                    input_buffer[buffer_pos - 3] == 'f' &&
                    input_buffer[buffer_pos - 2] == 'o' &&
                    input_buffer[buffer_pos - 1] == '\n')
                {
                    info();
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
                    //list_dir(all_files);
                    bitmap_draw_string("okok\n", BLACK);
                    buffer_pos -= 2; // Adjust buffer position to account for replacement
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

                if (buffer_pos >= 3 &&
                    input_buffer[buffer_pos - 3] == 'c' &&
                    input_buffer[buffer_pos - 2] == 'd' &&
                    input_buffer[buffer_pos - 1] == '\n')
                {
                    VDKChangeDir("VIPERRRRRRRRRRRRRRRRRRR MY GUYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY HOW ARE YOU?");
                    buffer_pos -= 2; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == 'd')
                {
                    mx++;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == 's')
                {
                    my++;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == 'w')
                {
                    my--;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == 'a')
                {
                    mx--;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }
            }
            else
            {
                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '1' && servicePage == FALSE)
                {
                    bitmap_draw_string("\nCONTINUED\n", BLACK);
                    continueSetup = TRUE;
                    pageNumber = 1;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '2' && servicePage == FALSE)
                {
                    bitmap_draw_string("\nQUIT SETUP\n", BLACK);
                    isInput = FALSE;
                    poweroff_sys = TRUE;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if(servicePage == TRUE)
                {
                    if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '1')
                    {
                        pageNumber = 4;
                        buffer_pos -= 1; // Adjust buffer position to account for replacement
                    }
                }

                if(agreePage == TRUE)
                {
                    if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == 'l')
                    {
                        pageNumber = 5;
                        buffer_pos = 1; // Adjust buffer position to account for replacement
                    }
                }
            }
        }
    }
}


void welcome()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
	printf("[OK] Starting VDK...\n");

	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

	printf("Welcome to ViaOS 1.0.0\n");
	printf("KERNEL VERSION 0.8\n");
	printf("Copyright 2024 Via\n");
}

void *memset(void *dst, char c, uint32_t n) {
    char *temp = dst;
    for (; n != 0; n--) *temp++ = c;
    return dst;
}

void *memcpy(void *dst, const void *src, uint32_t n) {
    char *ret = dst;
    char *p = dst;
    const char *q = src;
    while (n--)
        *p++ = *q++;
    return ret;
}

int memcmp(uint8_t *s1, uint8_t *s2, uint32_t n) {
    while (n--) {
        if (*s1 != *s2)
            return 0;
        s1++;
        s2++;
    }
    return 1;
}

void strcat(char *dest, const char *src) {
    char *end = (char *)dest + strlen(dest);
    memcpy((void *)end, (void *)src, strlen(src));
    end = end + strlen(src);
    *end = '\0';
}

void itoa(int num, char* str, int base) {
    int i = 0;
    int is_negative = 0;

    // Handle 0 explicitly
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Handle negative numbers only if base is 10
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // Append negative sign for negative numbers
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate string

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int strcmp(const char *s1, char *s2) {
    int i = 0;

    while ((s1[i] == s2[i])) {
        if (s2[i++] == 0)
            return 0;
    }
    return 1;
}

void VDK_InterpretFile(struct RIFS_File e)
{
    char idSh[MAX_FILENAME];
    char timeCreatedH[MAX_FILENAME];
    char timeCreatedS[MAX_FILENAME];
    char timeCreatedM[MAX_FILENAME];

    itoa(e.id, idSh, 10);
    itoa(e.time_created.hours, timeCreatedH, 10);
    itoa(e.time_created.mins, timeCreatedM, 10);
    itoa(e.time_created.secs, timeCreatedS, 10);

    bitmap_draw_string("Name ", BLACK);
    bitmap_draw_string(e.name, BLACK);
    printf("\n");

    printf("Time created ");
    printf(timeCreatedH);
    printf(" Hours ");
    printf(timeCreatedM);
    printf(" Minutes ");
    printf(timeCreatedS);
    printf(" Seconds\n");

    printf("Permissions ");
    printf("Read Only: ");
    if(e.permissions.ro == 1)
    {
        printf("Yes, ");
    }
    else
    {
        printf("No, ");
    }
    printf("Write Only: ");
    if(e.permissions.wo == 1)
    {
        printf("Yes, ");
    }
    else
    {
        printf("No, ");
    }
    printf("System file: ");
    if(e.permissions.sys == 1)
    {
        printf("Yes, ");
    }
    else
    {
        printf("No, ");
    }
    printf("\n");
}

void VDK_ViewFile(struct RIFS_F e)
{
    printf("\n");
    printf(e.metadata.name);
    printf("\n");
    printf(e.data);
    printf("\n");
}

void in_tos()
{
        bitmap_draw_string("Last updated 9:00PM IST 8/23/2024\n\n", BLACK);
        bitmap_draw_string(" \nIntroduction\n", BLACK);
        bitmap_draw_string("If you download or use our Operating System (ViaOS), or if you click to accept these Terms of Service, that means you agree to this TOS and will have to abide by our terms, so please read these terms **CAREFULLY**. If you are under the age of consent or a minor, it is recommended to have a parent/guardian read through these terms with you or have them explain it to you before accepting this.\n", BLACK);
        bitmap_draw_string("What you can and can't do with our software\n\n", BLACK);
        bitmap_draw_string("When you download our software (This can include ViaOS, pre-installed software, and more), you aren't allowed to:\n", BLACK);
        bitmap_draw_string("- Sell or give copies of ViaOS (This OS is literally free);\n", BLACK);
        // stops
}

void in_tos2()
{
        bitmap_draw_string("- Make commercial use of ViaOS;\n- Sell it in blacklisted nations (This can include nations that have our OS banned and embargoed nations);\n- And use our software for illegal or malicious intent.\nOtherwise, you could customize your copy of ViaOS (But not distribute it or sell it), use it for projects, and basically anything (That complies with this TOS and laws of course)\n", BLACK);
        bitmap_draw_string("\nPrivacy\n", BLACK);
        bitmap_draw_string("Your privacy is important to us, that means we can't:\n", BLACK);
        bitmap_draw_string("\n- Install spyware or malicious software on your PC or device;\n- Take your files away from you (Unless they're illegal or a TS breaker);\n", BLACK);
        // stops
}

void in_tos3()
{
        bitmap_draw_string("- And anything else that breaks internet/data privacy laws in your country.\n", BLACK);
        bitmap_draw_string("\nBut, when you break the TOS, we can:\n", BLACK);
        bitmap_draw_string("- Take away your copy(ies) of ViaOS;\n- Get you arrested if we manage to find anything illegal (And what we mean by 'if we manage to find anything illegal' is that if we find proof of you having illegal activity. We can't look into your personal data);\n- And threaten to sue\n", BLACK);
        bitmap_draw_string("\nConclusion\n", BLACK);
        bitmap_draw_string("Your local law may give you rights that this TOS cannot change. if so, this TOS applies as far as the law allows.\nIf we have or find reason to, we can change and edit or TOS from time to time. In that case we'll try to inform you of the change before it takes effect.\n", BLACK);
}

// FIXME: Help, somebody please fix this horrendous mouse driver
// Thanks Vule (x2)!
struct Point MousePosition;
#define PS2Leftbutton 0b00000001
#define PS2Middlebutton 0b00000010
#define PS2Rightbutton 0b00000100
#define PS2XSign 0b00010000
#define PS2YSign 0b00100000
#define PS2XOverflow 0b01000000
#define PS2YOverflow 0b10000000

void WaitMouse()
{
    uint64_t timeout = 100000;
    while (timeout--)
    {
        if ((ioport_in(0x64) & 0b10) == 0)
        {
            return;
        }
    }
}

void WaitInput()
{
    uint64_t timeout = 100000;
    while (timeout--)
    {
        if (ioport_in(0x64) & 0b1)
        {
            return;
        }
    }
}

uint8_t ReadMouse()
{
    WaitInput();
    return ioport_in(0x60);
}

void WriteMouse(uint8_t value)
{
    WaitMouse();
    ioport_out(0x64, 0xD4);
    WaitMouse();
    ioport_out(0x60, value);
}

void set_mouse_rate(uint8_t rate) 
{
    uint8_t status;

    ioport_out(0x60, 0xF3);
    status = ReadMouse();

    ioport_out(0x60, rate);
    status = ReadMouse();
}

void mouse_init()
{
    // Thanks Vule!
    ioport_out(0x64, 0xA8);

    set_mouse_rate(10);

    WaitMouse();
    ioport_out(0x64, 0x20);
    WaitInput();
    uint8_t status = ioport_in(0x60);
    status |= 0b10;
    WaitMouse();
    ioport_out(0x64, 0x60);
    WaitMouse();
    ioport_out(0x60, status);

    WriteMouse(0xF6);
    ReadMouse();

    WriteMouse(0xF4);
    ReadMouse();
}

int MousePacketReady = 0;
uint8_t MousePacket[4];
uint8_t MouseCycle;

void HandlePS2Mouse(uint8_t data)
{

    switch(MouseCycle){
        case 0:
            if (MousePacketReady) break;
            if (data & 0b00001000 == 0) break;
            MousePacket[0] = data;
            MouseCycle++;
            break;
        case 1:
            if (MousePacketReady) break;
            MousePacket[1] = data;
            MouseCycle++;
            break;
        case 2:
            if (MousePacketReady) break;
            MousePacket[2] = data;
            MousePacketReady = 1;
            MouseCycle = 0;
            break;
    }
}

enum BOOL_T bonzi = FALSE;

void ProcessMousePacket()
{
    if (!MousePacketReady) return;

        enum BOOL_T xNegative, yNegative, xOverflow, yOverflow;

        if (MousePacket[0] & PS2XSign){
            xNegative = TRUE;
        }else xNegative = FALSE;

        if (MousePacket[0] & PS2YSign){
            yNegative = TRUE;
        }else yNegative = FALSE;

        if (MousePacket[0] & PS2XOverflow){
            xOverflow = TRUE;
        }else xOverflow = FALSE;

        if (MousePacket[0] & PS2YOverflow){
            yOverflow = TRUE;
        }else yOverflow = FALSE;

        if (!xNegative){
            MousePosition.X += MousePacket[1] / 5;
            if (xOverflow){
                MousePosition.X += 255;
            }
        } else
        {
            MousePacket[1] = 256 - MousePacket[1];
            MousePosition.X -= MousePacket[1];
            if (xOverflow){
                MousePosition.X -= 255;
            }
        }

        if (!yNegative){
            MousePosition.Y -= MousePacket[2] / 5;
            if (yOverflow){
                MousePosition.Y -= 255;
            }
        } else
        {
            MousePacket[2] = 256 - MousePacket[2];
            MousePosition.Y += MousePacket[2];
            if (yOverflow){
                MousePosition.Y += 255;
            }
        }

        if (MousePosition.X < 0) MousePosition.X = 0;
        if (MousePosition.X > VGA_MAX_WIDTH-8) MousePosition.X = VGA_MAX_WIDTH-8;
        
        if (MousePosition.Y < 0) MousePosition.Y = 0;
        if (MousePosition.Y > VGA_MAX_HEIGHT-16) MousePosition.Y = VGA_MAX_HEIGHT-16;

        if(MousePacket[0] & PS2Leftbutton)
        {
            bonzi = TRUE;
        }
        else
        {
            bonzi = FALSE;
        }

        MousePacketReady = 0;
}

void mouse_fricker()
{

}

/* GIMP RGB C-Source image dump (hi.c) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  char          *comment;
  unsigned char  pixel_data[20 * 20 * 3 + 1];
} iconSt = {
  20, 20, 3,
  "Created with GIMP",
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\347\347\347\252\252\252\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\347\347\347eee\000\000\000\252"
  "\252\252\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\334\334\334\061\061"
  "\061\000\000\000uuu\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\341\341\341"
  "\063\063\063\000\000\000rrr\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\241\241"
  "\241\356\356\356\377\377\377\377\377\377\377\377\377\377\377\377\341\341"
  "\341:::\000\000\000ggg\364\364\364\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377zzz\000\000\000<<<\356\356"
  "\356\377\377\377\377\377\377\377\377\377\341\341\341@@@\000\000\000bbb\364\364"
  "\364\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\302\302\302\001\001\001\000\000\000\000\000\000\201\201\201\377\377\377\377"
  "\377\377\377\377\377\364\364\364UUU\000\000\000MMM\235\235\235\274\274\274\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\244"
  "\244\244\001\001\001\000\000\000\000\000\000\241\241\241\377\377\377\377\377\377\377\377\377"
  "\377\377\377ggg\000\000\000\025\025\025\000\000\000\000\000\000iii\377\377\377\377\377\377\377"
  "\377\377\377\377\377\376\376\376\350\350\350xxx%%%\241\241\241\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377^^^\000\000\000\024\024\024jjj\034"
  "\034\034\000\000\000\221\221\221\377\377\377\377\377\377\377\377\377\365\365\365"
  "\274\274\274SSS\265\265\265\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377^^^\000\000\000\004\004\004\226\226\226GGG\000\000\000\245\245\245"
  "\377\377\377\377\377\377\377\377\377\377\377\377\253\253\253\020\020\020\062"
  "\062\062\327\327\327\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377^^^\000\000\000\022\022\022\304\304\304ggg\000\000\000\225\225\225\377\377\377\377\377"
  "\377\377\377\377\377\377\377\231\231\231\001\001\001\002\002\002\231\231\231\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377lll\000\000\000FFF\351\351"
  "\351vvv\000\000\000\225\225\225\377\377\377\377\377\377\377\377\377\377\377\377"
  "uuu\000\000\000\000\000\000uuu\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\211\211\211\000\000\000\203\203\203\377\377\377\203\203\203\000\000\000\211\211"
  "\211\377\377\377\377\377\377\377\377\377\377\377\377ppp\000\000\000\000\000\000\\\\\\"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\211\211\211"
  "\000\000\000\211\211\211\377\377\377\211\211\211\000\000\000\211\211\211\377\377\377"
  "\377\377\377\377\377\377\377\377\377ppp\000\000\000\000\000\000\\\\\\\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\211\211\211\000\000\000\211\211\211"
  "\377\377\377\211\211\211\000\000\000\211\211\211\377\377\377\377\377\377\377\377"
  "\377\377\377\377ppp\000\000\000\000\000\000AAA\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\274\274\274\000\000\000\274\274\274\377\377\377\274\274\274"
  "\000\000\000\274\274\274\377\377\377\377\377\377\377\377\377\377\377\377\246\246"
  "\246\002\002\002\000\000\000ccc\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\274\274\274\377\377\377\377\377\377\377\377\377\274\274"
  "\274\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377yyy\000\000\000\274\274\274\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\274\274\274\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377",
};

VGA_COLOR_TYPE rgb_to_vga_color(uint8_t r, uint8_t g, uint8_t b) 
{
    // Simple mapping for demonstration purposes
    if (r > 128 && g > 128 && b > 128) return WHITE; // Light colors to WHITE
    else return BLACK; // Default to BLACK
}

void display_image(const struct {
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_pixel;
    unsigned char pixel_data[20 * 20 * 3 + 1];
} *icon) {
    if (icon->bytes_per_pixel != 3) {
        // Unsupported format
        return;
    }

    for (unsigned int y = 0; y < icon->height; y++) {
        for (unsigned int x = 0; x < icon->width; x++) {
            unsigned int pixel_index = (y * icon->width + x) * icon->bytes_per_pixel;
            uint8_t r = icon->pixel_data[pixel_index];
            uint8_t g = icon->pixel_data[pixel_index + 1];
            uint8_t b = icon->pixel_data[pixel_index + 2];
            
            VGA_COLOR_TYPE color = rgb_to_vga_color(r, g, b);
            vga_graphics_putpixel(x, y, color);
        }
    }
}

void kmain()
{
	/* Initialize everything */
	init();
	init_idt();
	if(isInstalled == FALSE)
		kb_init();
    ata_init();
    if(isInstalled == TRUE)
        mouse_init();
	welcome();
	enable_interrupts();

    vga_graphics_init();
    vga_graphics_clear_color(BLUE);

    const int DRIVE = ata_get_drive_by_model("QEMU HARDDISK"); // qemu on top
    const uint32_t LBA = 0;
    const uint8_t NO_OF_SECTORS = 1;
    char buf[ATA_SECTOR_SIZE] = {0};

    char data[512];
    char data2[512];
    char data3[512];

    // create file and write to drive
    struct RIFS_File e;
    struct RIFS_F bootSYS;
    perm_t exP;
    time_t exT;

    exP.ro = 1;
    exP.wo = 0;
    exP.sys = 1;

    exT.hours = 0;
    exT.mins = 0;
    exT.secs = 10;

    e.id = 0;
    e.permissions = exP;
    e.time_created = exT;
    e.time_modified = exT;
    e.time_accessed = exT;
    strcpy(e.name, "boot.sys");
    strcpy(data2, "INSTALLED\n/\n");

    bootSYS.metadata = e;
    strcpy(bootSYS.data, data2);

    struct RIFS_File e2;
    struct RIFS_F welcomeTXT;    

    exP.ro = 0;
    exP.wo = 0;
    exP.sys = 1;

    exT.hours = 1;
    exT.mins = 50;
    exT.secs = 42;

    e2.id = 1;
    e2.permissions = exP;
    e2.time_created = exT;
    e2.time_modified = exT;
    e2.time_accessed = exT;
    strcpy(e2.name, "welcome.txt");
    strcpy(data, "Welcome to ViaOS 1! This is a text file created automatically by ViaOS.\nWe hope you enjoy ViaOS!\n");

    welcomeTXT.metadata = e2;
    strcpy(welcomeTXT.data, data);

    struct RIFS_File e3;
    struct RIFS_F desktopCFG;    

    exP.ro = 0;
    exP.wo = 0;
    exP.sys = 1;

    exT.hours = 0;
    exT.mins = 0;
    exT.secs = 16;

    e3.id = 2;
    e3.permissions = exP;
    e3.time_created = exT;
    e3.time_modified = exT;
    e3.time_accessed = exT;
    strcpy(e3.name, "desktop.cfg");
    strcpy(data3, "USER: DEFAULTUSER0\n");

    desktopCFG.metadata = e3;
    strcpy(desktopCFG.data, data3);

    // Read boot.sys off disk and check if ViaOS is installed
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

    if(strcmp(buf, "INSTALLED\n/\n") == 0)
    {
        bitmap_draw_string("Verified ViaOS Installation\n", BLACK);
        isInstalled = TRUE;
    }

    // Read boot.sys off disk

    if(isInstalled == TRUE)
    {
        vga_graphics_clear_color(WHITE);
        vga_graphics_fill_rect(20, 24, 50, 50, BLUE);

        //in_tos();
    }

    // write files to drive

    // boot.sys
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &e, sizeof(e));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf, &data2, sizeof(data2));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

    // welcome.txt
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &e2, sizeof(e2));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 3, (uint32_t)buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf, &data, sizeof(data));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 4, (uint32_t)buf);

    // desktop.cfg
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &e3, sizeof(e3));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 5, (uint32_t)buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf, &data3, sizeof(data3));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 6, (uint32_t)buf);

    // write files to drive

    // read drive

    // boot.sys
    // memset(buf, 0, sizeof(buf));
    // ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);
    // e.id = 29;
    // memcpy(&e, buf, sizeof(e));

    //welcome.txt
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 3, (uint32_t)buf);
    e2.id = 99;
    memcpy(&e2, buf, sizeof(e2));

    // welcome data
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 4, (uint32_t)buf);
    memcpy(&data, buf, sizeof(data));

    // boot data
    //memset(buf, 0, sizeof(buf));
    //ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);
    //memcpy(&data2, buf, sizeof(data2));

    // desktop.cfg
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 5, (uint32_t)buf);

    memcpy(&e3, buf, sizeof(e3));

    // desktop data
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 6, (uint32_t)buf);
    memcpy(&data3, buf, sizeof(data3));

    // read drive

    // Add files to table
    all_files[0] = welcomeTXT;
    all_files[1] = bootSYS;
    all_files[2] = desktopCFG;
    // Add files to table

    //VDK_ViewFile(welcomeTXT);
    //VDK_ViewFile(bootSYS);
    //VDK_ViewFile(desktopCFG);

	while(1)
    {
        if(isInstalled == FALSE && stop1 == FALSE)
        {
            bitmap_draw_string(" ViaOS 1.0 Installer\n", BLACK);
            bitmap_draw_string("\nPress 1 to continue setup.\n", BLACK);
            bitmap_draw_string("Press 2 to exit setup.\n", BLACK);
            stop1 = TRUE;
        }

        if(pageNumber == 1 && stop2 == FALSE)
        {
            vga_graphics_clear_color(BLUE);
            reset_text_position();
            bitmap_draw_string("TERMS OF SERVICE -- PRESS 1 FOR NEXT PAGE.\n\n", BLACK);
            in_tos();
            servicePage = TRUE;
            stop2 = TRUE;
        }

        if(pageNumber == 4 && stop6 == FALSE)
        {
            vga_graphics_clear_color(BLUE);
            reset_text_position();
            bitmap_draw_string("TERMS OF SERVICE -- PRESS L TO SEE NEXT.\n\n", BLACK);
            in_tos2();
            agreePage = TRUE;
            stop6 = TRUE;
        }

        if(pageNumber == 5 && stop7 == FALSE)
        {
            vga_graphics_clear_color(BLUE);
            reset_text_position();
            bitmap_draw_string("TERMS OF SERVICE -- PRESS ANY KEY TO AGREE.\n\n", BLACK);
            in_tos3();
            agreePage2 = TRUE;
            serviceAgreed = TRUE;
            stop7 = TRUE;
        }

        if(serviceAgreed == TRUE && pageNumber == 2 && stop3 == FALSE)
        {
            vga_graphics_clear_color(BLACK);
            reset_text_position();

            bitmap_draw_string(" Hi, We are installing your system...\n", WHITE);

            // boot.sys
            memset(buf, 0, sizeof(buf));
            memcpy(buf, &e, sizeof(e));
            ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);

            memset(buf, 0, sizeof(buf));
            memcpy(buf, &data2, sizeof(data2));
            ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

            bitmap_draw_string(" We have successfully installed your system, Please reboot.\n", WHITE);

            stop3 = TRUE;
        }

        if(isInstalled == TRUE)
        {
            memset(BACK_BUFFER, 0, 200 * 320);
            
            vga_graphics_clear_color(BROWN);

            if(kbI == FALSE)
            {
	            HandlePS2Mouse(ioport_in(0x60));
	            ProcessMousePacket();
        	}

        	if(kbI == TRUE)
        	{
        		kbI == FALSE;
        	}
            reset_text_position();
            bitmap_draw_string(" Welcome to ViaOS. You are currently in \n VGA Mode 320x200. \n Thanks for choosing ViaOS.\n\n", BLACK);

            if(bonzi)
            {
                reset_text_position();
                bitmap_draw_string(" clicked \n", BLACK);
            }

            NPinit();
            NPdraw();

            vga_graphics_fill_rect(MousePosition.X, MousePosition.Y, 5, 5, BLACK);

            display_image(&iconSt);

            vga_swap_buffer();
        }

        // if(stop5 == FALSE)
        // {
        //     //VDK_InterpretFile(all_files[0].metadata);
        //     stop5 = TRUE;
        // }
    }
}