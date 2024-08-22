#include <via/stdio.h>

void hello()
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

	printf("\nHello!\n");
	printf("> ");
}