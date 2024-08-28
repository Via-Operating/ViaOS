#include "viaver.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"

// Create a handle, a status and a WNDCLASS.
struct HAND hwndV;
struct WND32_STATUS statV;
struct WNDCLASS ver;

void VRproc()
{
	set_text_position(100, 50);
    bitmap_draw_stringS(" ViaOS", YELLOW, TRUE, 76);
    set_text_position(100, 92);
    bitmap_draw_stringS(" Current Version: 0.8\n", YELLOW, TRUE, 92);
}

void VRinit()
{
	// Create a Via32 API Window.

	statV.x = 100;
	statV.y = 50;
	statV.w = 200;
	statV.h = 120;

	strcpy(hwndV.sName, " ViaVer");

	ver.handle = hwndV;
	ver.status = statV;

	// Set window foreground
    AllowSetForeground(WHITE);
}

void VRdraw()
{
	WND32_Paint(ver, VRproc);
}