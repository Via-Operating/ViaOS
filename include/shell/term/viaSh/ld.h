#include <via/stdio.h>

void list_dir()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
	printf("\n[ERROR] ");
	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
	printf("Unimplemented RIFS\n");

	printf("> ");
}