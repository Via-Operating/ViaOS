#include "window.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

int currentWND = 0;
VGA_COLOR_TYPE currentWNDC = CYAN;

// void AllowSetForeground(enum VGA_COLOR_TYPE e)
// {
// 	currentWNDC = e;
// }

void WND32_Paint(struct WNDCLASS wndClass, void (*proc)())
{
	vga_graphics_fill_rect(wndClass.status.x, wndClass.status.y, 100, 100, currentWNDC);
	set_text_position(wndClass.status.x, wndClass.status.y);
	bitmap_draw_stringS(wndClass.handle.sName, BLACK, TRUE, wndClass.status.y);
	vga_graphics_draw_line(wndClass.status.x, wndClass.status.y + 16, wndClass.status.x + 100, wndClass.status.y + 16, WHITE);
	proc();
}

void PaintDesktop(struct WNDCLASS* wndClasses)
{
	for(int i = 0; i < currentWND; i++)
	{
		//WND32_Paint(wndClasses[i]);
	}
}