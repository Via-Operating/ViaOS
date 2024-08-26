#include "window.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

int currentWND = 0;
VGA_COLOR_TYPE currentWNDC = CYAN;
WND32_COM uMsg = 0;

void AllowSetForeground(uint8_t color)
{
 	currentWNDC = color;
}

void WND32_Paint(struct WNDCLASS wndClass, void (*proc)())
{
	wndClass.WND32_COL = currentWNDC;
	vga_graphics_fill_rect(wndClass.status.x, wndClass.status.y, wndClass.status.w, wndClass.status.h, currentWNDC);
	set_text_position(wndClass.status.x, wndClass.status.y);
	bitmap_draw_stringS(wndClass.handle.sName, BLACK, TRUE, wndClass.status.y);
	vga_graphics_draw_line(wndClass.status.x, wndClass.status.y + 16, wndClass.status.x + wndClass.status.w, wndClass.status.y + 16, WHITE);
	vga_graphics_fill_rect(wndClass.status.x + wndClass.status.w - 10, wndClass.status.y, 10, 10, RED);
	proc();
}

void PaintDesktop(struct WNDCLASS* wndClasses)
{
	for(int i = 0; i < currentWND; i++)
	{
		WND32_Paint(wndClasses[i], wndClasses[i].WND32_COL);
	}
}

int GetWND32_Msg()
{
	return uMsg;
}