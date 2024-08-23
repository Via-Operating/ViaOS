// ViaOS
// Kernel
// GNU GPL 3.0 License, Read LICENSE.TXT

#include <via/stdio.h>
#include <via/shell/term/viaSh/echo.h>
#include <via/shell/term/viaSh/help.h>
#include <via/shell/term/viaSh/hi.h>
#include <via/shell/term/viaSh/ld.h>
#include <via/shell/term/viaSh/cd.h>
#include <via/shell/term/viaSh/info.h>
#include <via/shell/term/viaSh/shutdown.h>
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
enum BOOL_T isInstalled = FALSE;
enum BOOL_T continueSetup = FALSE;
enum BOOL_T isInput = TRUE;
enum BOOL_T stop1 = FALSE;
enum BOOL_T stop2 = FALSE;
enum BOOL_T serviceAgreed = FALSE;
enum BOOL_T servicePage = FALSE;
enum BOOL_T stop3 = FALSE;
enum BOOL_T stop4 = FALSE;
enum BOOL_T next = FALSE;

int pageNumber = 0;

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

extern void load_gdt();
extern void keyboard_handler();
extern void ata_handler();
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
                    list_dir(all_files);
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
            }
            else
            {
                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '1' && servicePage == FALSE)
                {
                    printf("\nCONTINUED\n");
                    continueSetup = TRUE;
                    pageNumber = 1;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '2' && servicePage == FALSE)
                {
                    printf("\nQUIT SETUP\n");
                    isInput = FALSE;
                    poweroff_sys = TRUE;
                    buffer_pos -= 1; // Adjust buffer position to account for replacement
                }

                if(servicePage == TRUE)
                {
                    if (buffer_pos >= 1 &&
                    input_buffer[buffer_pos - 1] == '1')
                    {
                        serviceAgreed = TRUE;
                        pageNumber = 2;
                        buffer_pos = 0; // Adjust buffer position to account for replacement
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

    printf("Name ");
    printf(e.name);
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
    printf("Last updated 9:00PM IST 8/23/2024\n\n");
    printf("Introduction\n");
    printf("If you download or use our Operating System (ViaOS), or if you click to accept these Terms of Service, that means you agree to this TOS and will have to abide by our terms, so please read these terms **CAREFULLY**. If you are under the age of consent or a minor, it is recommended to have a parent/guardian read through these terms with you or have them explain it to you before accepting this.\n");
    printf("What you can and can't do with our software\n\n");
    printf("When you download our software (This can include ViaOS, pre-installed software, and more), you aren't allowed to:\n");
    printf("- Sell or give copies of ViaOS (This OS is literally free);\n- Make commercial use of ViaOS;\n- Sell it in blacklisted nations (This can include nations that have our OS banned and embargoed nations);\n- And use our software for illegal or malicious intent.\nOtherwise, you could customize your copy of ViaOS (But not distribute it or sell it), use it for projects, and basically anything (That complies with this TOS and laws of course)\n");
    printf("\nPrivacy\n");
    printf("Your privacy is important to us, that means we can't:\n");
    printf("\n- Install spyware or malicious software on your PC or device;\n- Take your files away from you (Unless they're illegal or a TS breaker);\n- And anything else that breaks internet/data privacy laws in your country.\n");
    printf("\nBut, when you break the TOS, we can:\n");
    printf("- Take away your copy(ies) of ViaOS;\n- Get you arrested if we manage to find anything illegal (And what we mean by 'if we manage to find anything illegal' is that if we find proof of you having illegal activity. We can't look into your personal data);\n- And threaten to sue\n");
    printf("\nConclusion\n");
    printf("Your local law may give you rights that this TOS cannot change. if so, this TOS applies as far as the law allows.\nIf we have or find reason to, we can change and edit or TOS from time to time. These reasons can include: updates to our OS, our company practices, and our legal obligation. But those changes will be effective only to the extent that they can legally apply. In that case we'll try to inform you of the change before it takes effect.\n");
}

void kmain()
{
	/* Initialize everything */
	init();
	init_idt();
	kb_init();
    ata_init();
	welcome();
	enable_interrupts();

    const int DRIVE = ata_get_drive_by_model("VBOX HARDDISK");
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
    strcpy(data3, "NULL");

    desktopCFG.metadata = e3;
    strcpy(desktopCFG.data, data3);

    // Read boot.sys off disk and check if ViaOS is installed
    memset(buf, 0, sizeof(buf));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

    if(strcmp(buf, "INSTALLED\n/\n") == 0)
    {
        printf("Verified ViaOS Installation\n");
        isInstalled = TRUE;
    }

    // Read boot.sys off disk

    // write files to drive

    // boot.sys
    // memset(buf, 0, sizeof(buf));
    // memcpy(buf, &e, sizeof(e));
    // ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);

    // memset(buf, 0, sizeof(buf));
    // memcpy(buf, &data2, sizeof(data2));
    // ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

    // welcome.txt
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &e2, sizeof(e2));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 3, (uint32_t)buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf, &data, sizeof(data));
    ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 4, (uint32_t)buf);

    // desktop.cfg
    // memset(buf, 0, sizeof(buf));
    // memcpy(buf, &e3, sizeof(e3));
    // ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 5, (uint32_t)buf);

    // memset(buf, 0, sizeof(buf));
    // memcpy(buf, &data3, sizeof(data3));
    // ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 6, (uint32_t)buf);

    // write files to drive

    // read drive

    // boot.sys
    // memset(buf, 0, sizeof(buf));
    // ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);
    // e.id = 29;
    // memcpy(&e, buf, sizeof(e));

    // welcome.txt
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
            printf("\nViaOS 1.0 Installer\n");
            printf("\nPress 1 to continue setup.\n");
            printf("Press 2 to exit setup.\n");
            stop1 = TRUE;
        }

        if(pageNumber == 1 && stop2 == FALSE)
        {
            printf("TERMS OF SERVICE -- PRESS 1 TO AGREE.\n\n");
            in_tos();
            servicePage = TRUE;
            stop2 = TRUE;
        }

        if(serviceAgreed == TRUE && pageNumber == 2 && stop3 == FALSE)
        {
            terminal_initialize();
            printf("Hi, We are installing your system...\n");

            // boot.sys
            memset(buf, 0, sizeof(buf));
            memcpy(buf, &e, sizeof(e));
            ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 1, (uint32_t)buf);

            memset(buf, 0, sizeof(buf));
            memcpy(buf, &data2, sizeof(data2));
            ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA + 2, (uint32_t)buf);

            printf("We have successfully installed your system, Please reboot.\n");

            stop3 = TRUE;
        }
    }
}