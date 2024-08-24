#ifndef VGA_H
#define VGA_H

#include <via/stdio.h>

#define VGA_ADDRESS 0xA0000
#define VGA_MAX_WIDTH 320
#define VGA_MAX_HEIGHT 200
#define VGA_MAX (VGA_MAX_WIDTH * VGA_MAX_HEIGHT)

enum g_vga_color {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    GREY,
    DARK_GREY,
    BRIGHT_BLUE,
    BRIGHT_GREEN,
    BRIGHT_CYAN,
    BRIGHT_RED,
    BRIGHT_MAGENTA,
    YELLOW,
    WHITE,
};

typedef enum g_vga_color VGA_COLOR_TYPE;

// Miscellaneous Output
#define VGA_MISC_READ  0x3CC
#define VGA_MISC_WRITE 0x3C2

// Sequencer Registers
#define VGA_SEQ_INDEX  0x3C4
#define VGA_SEQ_DATA   0x3C5

// Graphics Controller Registers
#define VGA_GC_INDEX  0x3CE
#define VGA_GC_DATA   0x3CF

// Attribute Controller Registers
#define VGA_AC_INDEX  0x3C0
#define VGA_AC_READ   0x3C1
#define VGA_AC_WRITE  0x3C0

// CRT Controller Registers
#define VGA_CRTC_INDEX  0x3D4
#define VGA_CRTC_DATA   0x3D5

// VGA Color Palette Registers
#define VGA_DAC_READ_INDEX   0x3C7
#define VGA_DAC_WRITE_INDEX  0x3C8
#define VGA_DAC_DATA   0x3C9

// General Control and Status Registers
#define VGA_INSTAT_READ   0x3DA

#define BITMAP_SIZE 20

void vga_disable_cursor();

void vga_graphics_init();
void vga_graphics_clear_color(uint8_t color);
void vga_graphics_putpixel(uint16_t x, uint16_t y, uint8_t color);
void vga_graphics_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void vga_graphics_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
void vga_graphics_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
void vga_graphics_draw_circle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
void vga_graphics_scroll_up(uint16_t rows, uint8_t color);
void vga_graphics_scroll_down(uint16_t rows, uint8_t color);
void update_text_position();

void bitmap_draw_string(const char *str, uint8_t color);
void bitmap_putchar(char ch, uint8_t c);

#endif