#include "paint.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

// Create a handle, a status and a WNDCLASS.
struct HAND P_hwnd;
struct WND32_STATUS P_stat;
struct WNDCLASS P_app;

void Pproc()
{
	set_text_position(100, 50);
    bitmap_draw_stringS(" This app is", RED, TRUE, 76);
    set_text_position(100, 92);
    bitmap_draw_stringS("Currently Unimplemented", RED, TRUE, 92);
}

void PCinit()
{
	// Create a Via32 API Window.

	P_stat.x = 100;
	P_stat.y = 50;
	P_stat.w = 200;
	P_stat.h = 120;

	strcpy(P_hwnd.sName, " Paint Canvas");

	P_app.handle = P_hwnd;
	P_app.status = P_stat;

	// Set window foreground
    AllowSetForeground(WHITE);
}

void PCdraw()
{
	WND32_Paint(P_app, Pproc);
}