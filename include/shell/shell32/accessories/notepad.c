#include "notepad.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

// Create a handle, a status and a WNDCLASS.
struct HAND hwnd;
struct WND32_STATUS stat;
struct WNDCLASS notepad;

void NPproc()
{
	set_text_position(100, 50);
    bitmap_draw_stringS(" This app is", RED, TRUE, 76);
    set_text_position(100, 92);
    bitmap_draw_stringS(" currently unimplemented.\n", RED, TRUE, 92);
}

void NPinit()
{
	// Create a Via32 API Window.

	// Configure status and handle.
	stat.x = 100;
	stat.y = 50;
	stat.w = 200;
	stat.h = 120;

	strcpy(hwnd.sName, " Notepad");

	notepad.handle = hwnd;
	notepad.status = stat;

	// Set window foreground
    AllowSetForeground(YELLOW);
}

void NPdraw()
{
	WND32_Paint(notepad, NPproc);
}