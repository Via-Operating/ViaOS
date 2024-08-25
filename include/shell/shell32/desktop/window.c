#include "window.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

int currentWND = 0;

void WND32_Paint(struct WNDCLASS wndClass, void (*proc)())
{
	vga_graphics_fill_rect(wndClass.status.x, wndClass.status.y, 100, 100, CYAN);
	set_text_position(wndClass.status.x, wndClass.status.y);
	bitmap_draw_stringS(wndClass.handle.sName, BLACK, TRUE, wndClass.status.y);
	proc();
}

void PaintDesktop(struct WNDCLASS* wndClasses)
{
	for(int i = 0; i < currentWND; i++)
	{
		//WND32_Paint(wndClasses[i]);
	}
}