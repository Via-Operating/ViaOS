#ifndef STDIO_H
#define STDIO_H

#include <via/stddef.h>

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

	typedef struct
    {
        int hours;
        int mins;
        int secs;
    } time_t;

    typedef struct
    {
        int ro; // read only
        int wo; // write only
        int sys; // system file
    } perm_t;

    struct RIFS_File
    {
        int id;
        perm_t permissions;
        time_t time_created;
        time_t time_modified;
        time_t time_accessed;
        char name[32];
    };

    struct RIFS_F
    {
        struct RIFS_File metadata;
        char data[512];
    };

int strcpy(char *dst, const char *src);

extern char ioport_in(unsigned short port); // uint16_t port
extern void ioport_out(unsigned short port, unsigned char data);

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);

static inline uint16_t vga_entry(unsigned char uc, uint8_t color);

size_t strlen(const char* str);

void terminal_initialize(void);

void terminal_setcolor(uint8_t color);

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

void terminal_putchar(char c);

void terminal_write(const char* data, size_t size);

void printf(const char* data);

void idt_set_entry(int index, uint32_t base, uint8_t flags);

void VDK_InterpretFile(struct RIFS_File e);
void VDK_ViewFile(struct RIFS_F e);

void itoa(int num, char* str, int base);
int strcmp(const char *s1, char *s2);
void *memset(void *dst, char c, uint32_t n);
void *memcpy(void *dst, const void *src, uint32_t n);

#endif